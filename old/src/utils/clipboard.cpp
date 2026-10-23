#include <utils/clipboard.hpp>
#include <utils/logger.hpp>

#include <array>
#include <cstdio>
#include <cstring>
#include <format>
#include <memory>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

namespace
{
    struct PipeCloser
    {
        void operator()(FILE *f) const noexcept
        {
            if (f)
                ::pclose(f);
        }
    };

    std::optional<std::string> run_capture(const char *cmd)
    {
        std::unique_ptr<FILE, PipeCloser> pipe(popen(cmd, "r"));
        if (!pipe)
            return std::nullopt;

        std::string out;
        char buf[4096];
        while (std::fgets(buf, sizeof(buf), pipe.get()) != nullptr)
            out += buf;

        if (out.empty())
            return std::nullopt;
        return out;
    }

    bool tool_exists(const char *name)
    {
        const std::string cmd = std::format("command -v {} >/dev/null 2>&1", name);
        return std::system(cmd.c_str()) == 0;
    }

    // argv[0]=prog, rest args, nullptr-terminated. Feed `text` on stdin.
    bool exec_write(const char *const argv[], std::string_view text)
    {
        int pipefd[2];
        if (::pipe(pipefd) != 0)
            return false;

        const pid_t pid = ::fork();
        if (pid < 0)
        {
            ::close(pipefd[0]);
            ::close(pipefd[1]);
            return false;
        }
        if (pid == 0)
        {
            ::close(pipefd[1]);
            if (::dup2(pipefd[0], STDIN_FILENO) < 0)
                _exit(127);
            ::close(pipefd[0]);
            ::execvp(argv[0], const_cast<char *const *>(argv));
            _exit(127);
        }

        ::close(pipefd[0]);
        const char *p = text.data();
        std::size_t left = text.size();
        while (left > 0)
        {
            const ssize_t n = ::write(pipefd[1], p, left);
            if (n < 0)
            {
                if (errno == EINTR)
                    continue;
                break;
            }
            p += static_cast<std::size_t>(n);
            left -= static_cast<std::size_t>(n);
        }
        ::close(pipefd[1]);

        int status = 0;
        if (::waitpid(pid, &status, 0) < 0)
            return false;
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }

    std::string base64_encode(std::string_view in)
    {
        static constexpr char kTable[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out;
        out.reserve(((in.size() + 2) / 3) * 4);
        unsigned val = 0;
        int valb = -6;
        for (unsigned char c : in)
        {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0)
            {
                out.push_back(kTable[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6)
            out.push_back(kTable[((val << 8) >> (valb + 8)) & 0x3F]);
        while (out.size() % 4)
            out.push_back('=');
        return out;
    }
}

std::optional<std::string> Clipboard::read()
{
    static constexpr std::array<const char *, 3> cmds = {
        "wl-paste -n 2>/dev/null",
        "xclip -selection clipboard -o 2>/dev/null",
        "xsel --clipboard --output 2>/dev/null",
    };

    for (const char *cmd : cmds)
    {
        if (auto text = run_capture(cmd))
        {
            Logger::debug(std::format("clipboard: read {} bytes via {}", text->size(), cmd));
            return text;
        }
    }

    Logger::debug("clipboard: no clipboard tool returned text");
    return std::nullopt;
}

bool Clipboard::write_osc52(std::string_view text)
{
    const std::string b64 = base64_encode(text);
    const std::string seq = std::format("\033]52;c;{}\a", b64);
    const ssize_t n = ::write(STDOUT_FILENO, seq.data(), seq.size());
    if (n < 0 || static_cast<std::size_t>(n) != seq.size())
    {
        Logger::warning("clipboard: OSC52 write failed");
        return false;
    }
    Logger::debug(std::format("clipboard: OSC52 {} bytes", text.size()));
    return true;
}

bool Clipboard::write(std::string_view text, std::string *hint_out)
{
    if (tool_exists("wl-copy"))
    {
        const char *argv[] = {"wl-copy", nullptr};
        if (exec_write(argv, text))
        {
            Logger::debug(std::format("clipboard: wrote {} bytes via wl-copy", text.size()));
            return true;
        }
    }
    if (tool_exists("xclip"))
    {
        const char *argv[] = {"xclip", "-selection", "clipboard", nullptr};
        if (exec_write(argv, text))
        {
            Logger::debug(std::format("clipboard: wrote {} bytes via xclip", text.size()));
            return true;
        }
    }
    if (tool_exists("xsel"))
    {
        const char *argv[] = {"xsel", "-ib", nullptr};
        if (exec_write(argv, text))
        {
            Logger::debug(std::format("clipboard: wrote {} bytes via xsel", text.size()));
            return true;
        }
    }

    if (write_osc52(text))
    {
        if (hint_out)
            *hint_out = "Clipboard tools missing; used OSC52";
        Logger::warning("clipboard: no wl-copy/xclip/xsel — OSC52 fallback");
        return true;
    }

    if (hint_out)
        *hint_out = "Clipboard write failed (install wl-copy or xclip)";
    Logger::warning("clipboard: write failed (no tool, OSC52 failed)");
    return false;
}

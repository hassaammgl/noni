#include <extensions/builtin_hello.hpp>
#include <utils/logger.hpp>

#include <format>

namespace
{
    class HelloExtension : public Extension
    {
    public:
        std::string id() const override { return "noni.hello"; }
        std::string name() const override { return "Hello Extension"; }

        void activate(ExtensionContext &ctx) override
        {
            ctx_ = &ctx;

            ctx.register_command("extension.hello.ping", [this]() {
                if (ctx_)
                    ctx_->info("pong");
            });

            ctx.on_buffer_saved([this](Buffer &b) {
                Logger::debug(std::format(
                    "noni.hello: buffer saved {}",
                    b.get_buffer_path().empty() ? "(untitled)" : b.get_buffer_path().string()));
                (void)this;
            });

            // Optional config: extensions["noni.hello"].greet
            const auto cfg = ctx.extension_config();
            if (cfg.is_object())
            {
                const std::string greet = cfg.get_string("greet", "");
                if (!greet.empty())
                    ctx.info(greet);
            }
        }

        void deactivate() override { ctx_ = nullptr; }

    private:
        ExtensionContext *ctx_ = nullptr;
    };
}

std::unique_ptr<Extension> make_hello_extension()
{
    return std::make_unique<HelloExtension>();
}

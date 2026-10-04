#pragma once

#include <commands/command.hpp>
#include <configs/config.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct KeyToken
{
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
    int code = 0;
    char32_t codepoint = 0;

    bool operator==(const KeyToken &o) const
    {
        return ctrl == o.ctrl && alt == o.alt && shift == o.shift && code == o.code;
    }
};

struct ResolvedBinding
{
    std::vector<KeyToken> chord;
    CommandId command;
    std::string when;
    bool leader = false; // first key is Space
    std::string owner;   // empty or "noni.core" = config; extension id for runtime
};

// Resolves KeySequence → CommandId. Does not execute.
class KeybindingEngine
{
public:
    void load(const AppConfig &config);

    // Append a runtime binding owned by `owner` (extensions). Returns false if key invalid.
    bool add_binding(
        const std::string &key,
        const CommandId &command,
        const std::string &when = "",
        const std::string &owner = "");

    void remove_bindings_owned_by(const std::string &owner);

    ResolveResult resolve(int raw_key, const std::string &when_context, InputContext input_ctx);

    void clear_chord();
    bool has_pending() const { return !pending.empty(); }
    std::string pending_label() const;

    std::vector<CommandId> bound_commands() const;

    static KeyToken from_raw(int raw_key);
    static void register_extended_keys();
    // Bytes of a define_key sequence, or nullptr. Terminal replays these to the PTY.
    static const char *extended_seq(int raw_key);
    static std::vector<KeyToken> parse_key(const std::string &spec);
    static bool when_matches(const std::string &when, const std::string &context);
    ResolveResult resolve_token(KeyToken tok, const std::string &when_context, InputContext input_ctx);

private:
    struct FirstKey
    {
        int code = 0;
        bool ctrl = false;
        bool alt = false;
        bool shift = false;

        bool operator==(const FirstKey &o) const
        {
            return code == o.code && ctrl == o.ctrl && alt == o.alt && shift == o.shift;
        }
    };
    struct FirstKeyHash
    {
        std::size_t operator()(const FirstKey &k) const noexcept
        {
            return static_cast<std::size_t>(k.code) * 131u +
                   static_cast<std::size_t>(k.ctrl) +
                   static_cast<std::size_t>(k.alt) * 2u +
                   static_cast<std::size_t>(k.shift) * 4u;
        }
    };

    std::vector<ResolvedBinding> bindings;
    std::vector<KeyToken> pending;
    std::unordered_map<FirstKey, std::vector<std::size_t>, FirstKeyHash> first_index_;

    void rebuild_first_index();
    bool is_leader_token(const KeyToken &t) const;
    ResolveResult resolve_pending(const std::string &when_context, InputContext input_ctx);
};

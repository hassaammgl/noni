#pragma once

#include <string>

enum class RegisterType
{
    Character,
    Line,
};

struct RegisterValue
{
    std::string text;
    RegisterType type = RegisterType::Character;

    bool empty() const { return text.empty(); }
};

class RegisterFile
{
private:
    RegisterValue unnamed_;
    RegisterValue named_[26];
    char pending_ = '"';

    static int named_index(char name)
    {
        if (name >= 'a' && name <= 'z')
            return name - 'a';
        if (name >= 'A' && name <= 'Z')
            return name - 'A';
        return -1;
    }

public:
    void set_pending(char name) { pending_ = name; }
    char pending() const { return pending_; }
    void clear_pending() { pending_ = '"'; }

    void set(char name, RegisterValue value)
    {
        const int i = named_index(name);
        if (i >= 0)
            named_[static_cast<std::size_t>(i)] = value;
        unnamed_ = std::move(value);
    }

    void set_unnamed(RegisterValue value)
    {
        unnamed_ = std::move(value);
    }

    void yank_to_pending(RegisterValue value)
    {
        set(pending_, std::move(value));
        clear_pending();
    }

    const RegisterValue &get(char name) const
    {
        const int i = named_index(name);
        if (i >= 0)
            return named_[static_cast<std::size_t>(i)];
        return unnamed_;
    }

    const RegisterValue &unnamed() const { return unnamed_; }
    const RegisterValue &pending_value() const { return get(pending_); }
};

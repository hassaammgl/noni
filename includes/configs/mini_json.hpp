#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>

namespace MiniJson
{
    struct Value;

    using Object = std::map<std::string, Value>;
    using Array = std::vector<Value>;
    using Null = std::nullptr_t;

    struct Value
    {
        std::variant<Null, bool, double, std::string, Array, Object> data;

        bool is_null() const;
        bool is_bool() const;
        bool is_number() const;
        bool is_string() const;
        bool is_array() const;
        bool is_object() const;

        bool as_bool(bool fallback = false) const;
        int as_int(int fallback = 0) const;
        double as_number(double fallback = 0.0) const;
        const std::string &as_string() const;
        const Array &as_array() const;
        const Object &as_object() const;

        const Value *get(const std::string &key) const;
        std::string get_string(const std::string &key, const std::string &fallback = "") const;
        int get_int(const std::string &key, int fallback = 0) const;
    };

    Value parse(const std::string &text);
}

#include <configs/mini_json.hpp>

#include <cctype>
#include <stdexcept>
#include <string>

namespace MiniJson
{
    namespace
    {
        const std::string empty_string;
        const Array empty_array;
        const Object empty_object;

        void append_escaped(std::string &out, const std::string &s)
        {
            out.push_back('"');
            for (unsigned char c : s)
            {
                switch (c)
                {
                case '"':
                    out += "\\\"";
                    break;
                case '\\':
                    out += "\\\\";
                    break;
                case '\b':
                    out += "\\b";
                    break;
                case '\f':
                    out += "\\f";
                    break;
                case '\n':
                    out += "\\n";
                    break;
                case '\r':
                    out += "\\r";
                    break;
                case '\t':
                    out += "\\t";
                    break;
                default:
                    if (c < 0x20)
                    {
                        static const char *hex = "0123456789abcdef";
                        out += "\\u00";
                        out.push_back(hex[c >> 4]);
                        out.push_back(hex[c & 0xF]);
                    }
                    else
                    {
                        out.push_back(static_cast<char>(c));
                    }
                    break;
                }
            }
            out.push_back('"');
        }

        void stringify_into(std::string &out, const Value &v)
        {
            if (v.is_null())
            {
                out += "null";
                return;
            }
            if (v.is_bool())
            {
                out += v.as_bool() ? "true" : "false";
                return;
            }
            if (v.is_number())
            {
                const double n = v.as_number();
                if (n == static_cast<double>(static_cast<long long>(n)))
                    out += std::to_string(static_cast<long long>(n));
                else
                    out += std::to_string(n);
                return;
            }
            if (v.is_string())
            {
                append_escaped(out, v.as_string());
                return;
            }
            if (v.is_array())
            {
                out.push_back('[');
                bool first = true;
                for (const auto &el : v.as_array())
                {
                    if (!first)
                        out.push_back(',');
                    first = false;
                    stringify_into(out, el);
                }
                out.push_back(']');
                return;
            }
            if (v.is_object())
            {
                out.push_back('{');
                bool first = true;
                for (const auto &[k, val] : v.as_object())
                {
                    if (!first)
                        out.push_back(',');
                    first = false;
                    append_escaped(out, k);
                    out.push_back(':');
                    stringify_into(out, val);
                }
                out.push_back('}');
            }
        }
    }

    bool Value::is_null() const { return std::holds_alternative<Null>(data); }
    bool Value::is_bool() const { return std::holds_alternative<bool>(data); }
    bool Value::is_number() const { return std::holds_alternative<double>(data); }
    bool Value::is_string() const { return std::holds_alternative<std::string>(data); }
    bool Value::is_array() const { return std::holds_alternative<Array>(data); }
    bool Value::is_object() const { return std::holds_alternative<Object>(data); }

    bool Value::as_bool(bool fallback) const
    {
        return is_bool() ? std::get<bool>(data) : fallback;
    }

    int Value::as_int(int fallback) const
    {
        return is_number() ? static_cast<int>(std::get<double>(data)) : fallback;
    }

    double Value::as_number(double fallback) const
    {
        return is_number() ? std::get<double>(data) : fallback;
    }

    const std::string &Value::as_string() const
    {
        return is_string() ? std::get<std::string>(data) : empty_string;
    }

    const Array &Value::as_array() const
    {
        return is_array() ? std::get<Array>(data) : empty_array;
    }

    const Object &Value::as_object() const
    {
        return is_object() ? std::get<Object>(data) : empty_object;
    }

    const Value *Value::get(const std::string &key) const
    {
        if (!is_object())
            return nullptr;
        const auto &obj = as_object();
        const auto it = obj.find(key);
        return it == obj.end() ? nullptr : &it->second;
    }

    std::string Value::get_string(const std::string &key, const std::string &fallback) const
    {
        const Value *v = get(key);
        return (v && v->is_string()) ? v->as_string() : fallback;
    }

    int Value::get_int(const std::string &key, int fallback) const
    {
        const Value *v = get(key);
        return (v && v->is_number()) ? v->as_int(fallback) : fallback;
    }

    std::string stringify(const Value &v)
    {
        std::string out;
        out.reserve(256);
        stringify_into(out, v);
        return out;
    }
}

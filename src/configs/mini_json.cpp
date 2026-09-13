#include <configs/mini_json.hpp>

#include <cctype>
#include <stdexcept>
#include <string>

namespace MiniJson
{
    namespace
    {
        struct Parser
        {
            const std::string &text;
            std::size_t i = 0;

            void skip_ws()
            {
                while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i])))
                    ++i;
            }

            char peek() const
            {
                return i < text.size() ? text[i] : '\0';
            }

            char get()
            {
                return i < text.size() ? text[i++] : '\0';
            }

            Value parse_value()
            {
                skip_ws();
                const char c = peek();
                if (c == '{')
                    return parse_object();
                if (c == '[')
                    return parse_array();
                if (c == '"')
                    return Value{parse_string()};
                if (c == 't' || c == 'f')
                    return parse_bool();
                if (c == 'n')
                    return parse_null();
                if (c == '-' || std::isdigit(static_cast<unsigned char>(c)))
                    return parse_number();
                throw std::runtime_error("Invalid JSON");
            }

            Value parse_object()
            {
                get(); // {
                Object obj;
                skip_ws();
                if (peek() == '}')
                {
                    get();
                    return Value{std::move(obj)};
                }

                while (true)
                {
                    skip_ws();
                    if (peek() != '"')
                        throw std::runtime_error("Expected string key");
                    std::string key = parse_string();
                    skip_ws();
                    if (get() != ':')
                        throw std::runtime_error("Expected ':'");
                    obj.emplace(std::move(key), parse_value());
                    skip_ws();
                    const char sep = get();
                    if (sep == '}')
                        break;
                    if (sep != ',')
                        throw std::runtime_error("Expected ',' or '}'");
                }
                return Value{std::move(obj)};
            }

            Value parse_array()
            {
                get(); // [
                Array arr;
                skip_ws();
                if (peek() == ']')
                {
                    get();
                    return Value{std::move(arr)};
                }

                while (true)
                {
                    arr.push_back(parse_value());
                    skip_ws();
                    const char sep = get();
                    if (sep == ']')
                        break;
                    if (sep != ',')
                        throw std::runtime_error("Expected ',' or ']'");
                }
                return Value{std::move(arr)};
            }

            std::string parse_string()
            {
                if (get() != '"')
                    throw std::runtime_error("Expected '\"'");
                std::string out;
                while (i < text.size())
                {
                    const char c = get();
                    if (c == '"')
                        return out;
                    if (c == '\\')
                    {
                        const char e = get();
                        switch (e)
                        {
                        case '"':
                        case '\\':
                        case '/':
                            out.push_back(e);
                            break;
                        case 'n':
                            out.push_back('\n');
                            break;
                        case 't':
                            out.push_back('\t');
                            break;
                        case 'r':
                            out.push_back('\r');
                            break;
                        default:
                            out.push_back(e);
                            break;
                        }
                    }
                    else
                    {
                        out.push_back(c);
                    }
                }
                throw std::runtime_error("Unterminated string");
            }

            Value parse_bool()
            {
                if (text.compare(i, 4, "true") == 0)
                {
                    i += 4;
                    return Value{true};
                }
                if (text.compare(i, 5, "false") == 0)
                {
                    i += 5;
                    return Value{false};
                }
                throw std::runtime_error("Invalid bool");
            }

            Value parse_null()
            {
                if (text.compare(i, 4, "null") == 0)
                {
                    i += 4;
                    return Value{nullptr};
                }
                throw std::runtime_error("Invalid null");
            }

            Value parse_number()
            {
                const std::size_t start = i;
                if (peek() == '-')
                    get();
                while (std::isdigit(static_cast<unsigned char>(peek())))
                    get();
                if (peek() == '.')
                {
                    get();
                    while (std::isdigit(static_cast<unsigned char>(peek())))
                        get();
                }
                return Value{std::stod(text.substr(start, i - start))};
            }
        };

        static const std::string empty_string;
        static const Array empty_array;
        static const Object empty_object;
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

    Value parse(const std::string &text)
    {
        Parser p{text, 0};
        Value v = p.parse_value();
        p.skip_ws();
        if (p.i != text.size())
            throw std::runtime_error("Trailing JSON content");
        return v;
    }

    namespace
    {
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

    std::string stringify(const Value &v)
    {
        std::string out;
        out.reserve(256);
        stringify_into(out, v);
        return out;
    }
}

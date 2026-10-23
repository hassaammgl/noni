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
}

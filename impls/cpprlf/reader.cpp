#include "reader.hpp"
#include <exception>
#include <iomanip>
#include <optional>
#include <regex>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

struct reader {
    reader(std::vector<std::string> tokens):
        my_tokens{std::move(tokens)}, my_position{0} {}

    bool has_more() const { return my_position < my_tokens.size(); }

    std::optional<std::string> peek() const
    {
        if (my_position >= my_tokens.size()) {
            return std::nullopt;
        }
        return my_tokens.at(my_position);
    }

    std::optional<std::string> next()
    {
        auto token{peek()};
        if (token) ++my_position;
        return token;
    }

    std::string dump() const
    {
        std::ostringstream out;
        out << "position: " << my_position << ": ";
        for (const auto& token: my_tokens) {
            out << std::quoted(token) << ' ';
        }
        return out.str();
    }

    void throw_eof(
            const std::source_location location =
            std::source_location::current())
    {
        std::string message("Unexpected end of input in ");
        message += location.function_name();
        message += ": ";
        message += dump();
        throw std::runtime_error{message};
    }

private:
    const std::vector<std::string> my_tokens;
    std::size_t my_position;
};

std::vector<std::string> tokenize(std::string_view sv)
{
    // It is possible to get <regex> to work with string_views,
    // but it is simpler to use strings.
    // So, we copy the string_view into a string.
    const std::string s{sv};
    std::vector<std::string> tokens;
    const std::regex regex{R"RAW([\s,]*(~@|[\[\]{}()'`~^@]|"(?:\\.|[^\\"])*"?|;.*|[^\s\[\]{}('"`,;)]*))RAW"};
    std::sregex_iterator iter(s.begin(), s.end(), regex);
    const std::sregex_iterator end;
    for (; iter != end; ++iter) {
        tokens.push_back((*iter)[1].str());
    }
    return tokens;
}

std::optional<mal_type> try_number(const std::string& token)
{
    try {
        return std::stoi(token);
    } catch (const std::invalid_argument&) {
        return std::nullopt;
    } 
}

mal_type read_string(std::string_view s)
{
    /*
     * When a string is read, the following transformations are applied:
     *
     * a backslash followed by a doublequote is translated into a plain
     * doublequote character,
     *
     * a backslash followed by "n" is translated into a newline,
     *
     * and a backslash followed by another backslash is translated into a
     * single backslash.
     */
    if ((s.size() < 2) or ('"' != s.back())) {
        throw std::runtime_error{"unbalanced doublequote"};
    }
    // Remove the leading and trailing doublequotes.
    s.remove_prefix(1);
    s.remove_suffix(1);
    std::vector<char> v;
    bool backslash_seen{false};
    for (char c: s) {
        if (backslash_seen and ('n' == c)) {
            v.push_back('\n');
            backslash_seen = false;
        } else if (backslash_seen) {
            v.push_back(c);
            backslash_seen = false;
        } else if ('\\' == c) {
            backslash_seen = true;
        } else {
            v.push_back(c);
        }
    }
    if (backslash_seen) {
        // Using the phrase "end of input" to match what the tests expects.
        throw std::runtime_error{
            "unexpected end of input (string)"
        };
    }
    return v;
}

mal_type read_atom(reader& r)
{
    const auto token{r.next()};
    if (not token) r.throw_eof();
    if ((token->size() > 0) and ('"' == token->at(0))) {
        return read_string(*token);
    }
    if (auto number{try_number(*token)}; number) return *number;
    if ("nil" == *token) return mal_nil{};
    if ("false" == *token) return mal_false{};
    if ("true" == *token) return mal_true{};
    return *token;
}

//TODO: [] should create a vector instead of a list.
//      Vector and list can be the same but must be tagged.
//TODO: { a b c d } creates a map with entries a:b and c:d.
mal_type read_list(reader& r, char opener)
{
    mal_type read_form(reader& r);
    mal_list list(opener);
    const std::string closer_string(1, list.get_closer());
    r.next(); // Consume opener
    while (true) {
        const auto token{r.peek()};
        if (not token) r.throw_eof();
        if (closer_string == *token) {
            r.next(); // Consume closer.
            return list;
        }
        mal_list_add(list, read_form(r));
    }
}

mal_type read_form(reader& r)
{
    auto get_opener = [](const auto& token) -> char
    {
        if (token.empty()) return '\0';
        char c{token.at(0)};
        if (std::string{"([{"}.contains(c)) return c;
        return '\0';
    };

    const auto token{r.peek()};
    if (not token) r.throw_eof();
    const char opener{get_opener(*token)};
    if (not ('\0' == opener)) {
        return read_list(r, opener);
    } else {
        return read_atom(r);
    }
}

mal_type read_str(std::string_view s)
{
    reader r{tokenize(s)};
    return read_form(r);
}


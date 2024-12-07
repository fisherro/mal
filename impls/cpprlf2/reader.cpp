#include "reader.hpp"
#include "types.hpp"
#include <exception>
#if 0
#include <iomanip>
#endif
#include <optional>
#include <regex>
#include <source_location>
#if 0
#include <sstream>
#endif
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <print> // temporary

//TODO: Handle Keywords

const std::unordered_map<std::string, Symbol> reader_macros{
    { "@",  Symbol{"deref"} },
    { "'",  Symbol{"quote"} },
    { "`",  Symbol{"quasiquote"} },
    { "~",  Symbol{"unquote"} },
    { "~@", Symbol{"splice-unquote"} },
};

struct Reader {
    Reader(std::vector<std::string> tokens):
        _tokens{std::move(tokens)}, _position{0} {}

    bool has_more() const { return _position < _tokens.size(); }

    std::optional<std::string> peek() const
    {
        if (_position >= _tokens.size()) {
            return std::nullopt;
        }
        return _tokens.at(_position);
    }

    std::optional<std::string> next()
    {
        auto token{peek()};
        if (token) ++_position;
        return token;
    }

#if 0
    std::string dump() const
    {
        // Not using std::format here because std::quoted is nice.
        std::ostringstream out;
        out << "position: " << _position << ": ";
        for (const auto& token: _tokens) {
            out << std::quoted(token) << ' ';
        }
        return out.str();
    }
#endif

private:
    const std::vector<std::string> _tokens;
    std::size_t _position;
};

void throw_eof(
        const std::source_location location =
        std::source_location::current())
{
#if 0
    std::string message("Unexpected end of input in ");
    message += location.function_name();
    message += ": ";
    message += dump();
    throw std::runtime_error{message};
#else
    throw std::runtime_error{"unexpected end of input"};
#endif
}

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
        std::string token{(*iter)[1].str()};
        if ((token.size() > 0) and (';' == token[0])) continue;
        tokens.push_back(token);
    }
    if (tokens.empty()) throw Comment_exception{};
    return tokens;
}

std::optional<Value> try_number(const std::string& token)
{
    try {
        return Number{std::stoi(token)};
    } catch (const std::invalid_argument&) {
        return std::nullopt;
    } 
}

Value read_string(std::string_view in_string)
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
    if ((in_string.size() < 2) or ('"' != in_string.back())) {
        throw std::runtime_error{"unbalanced doublequote"};
    }
    // Remove the leading and trailing doublequotes.
    in_string.remove_prefix(1);
    in_string.remove_suffix(1);
    String out_string;
    bool backslash_seen{false};
    for (char c: in_string) {
        if (backslash_seen and ('n' == c)) {
            out_string.push_back('\n');
            backslash_seen = false;
        } else if (backslash_seen) {
            out_string.push_back(c);
            backslash_seen = false;
        } else if ('\\' == c) {
            backslash_seen = true;
        } else {
            out_string.push_back(c);
        }
    }
    if (backslash_seen) {
        // Using the phrase "end of input" to match what the tests expects.
        throw std::runtime_error{
            "unexpected end of input (string)"
        };
    }
    return out_string;
}

Value read_atom(Reader& r)
{
    const auto token_opt{r.next()};
    if (not token_opt) throw_eof();
    if ((token_opt->size() > 0) and ('"' == token_opt->at(0))) {
        return read_string(*token_opt);
    }
    if (auto number{try_number(*token_opt)}; number) return *number;
    if ("nil" == *token_opt)     return Nil{};
    if ("false" == *token_opt)   return False{};
    if ("true" == *token_opt)    return True{};
    if (':' == token_opt->at(0)) return Keyword{*token_opt};
    return Symbol{*token_opt};
}

template <typename T>
T read_seq(Reader& r, std::string_view closer)
{
    // Forward declaration of read_form:
    Value read_form(Reader& r);

    T seq;
    while (true) {
        const auto token_opt{r.peek()};
        if (not token_opt) throw_eof();
        if (token_opt->empty()) throw_eof();//??
        if (closer == *token_opt) {
            r.next(); // Consume closer.
            return seq;
        }
        seq.push_back(read_form(r));
    }
}

Value read_list(Reader& r)
{
    const auto opener_opt{r.next()};
    if (not opener_opt) throw_eof();
    if ("{" == *opener_opt) {
        return boxen_to_map(read_seq<List>(r, "}"));
    }
    if ("[" == *opener_opt) {
        return read_seq<Vector>(r, "]");
    }
    return read_seq<List>(r, ")");
}

Value read_form(Reader& r)
{
    auto is_opener = [](const auto& token) -> bool
    {
        if (token.empty()) return false;
        char c{token.at(0)};
        return std::string{"([{"}.contains(c);
    };

    const auto token_opt{r.peek()};
    if (not token_opt) throw_eof();
    if (is_opener(*token_opt)) {
        return read_list(r);
    }
    auto rm_iter{reader_macros.find(*token_opt)};
    if (reader_macros.end() != rm_iter) {
        r.next(); // Consume the token.
        List list;
        list.push_back(rm_iter->second);
        list.push_back(read_form(r));
        return list;
    }
    if ("^" == *token_opt) {
        r.next(); // Consume "^".
        List list;
        list.push_back(Symbol{"with-meta"});
        Value metadata{read_form(r)};
        Value object{read_form(r)};
        list.push_back(object);
        list.push_back(metadata);
        return list;
    }
    return read_atom(r);
}

Value read_str(std::string_view s)
{
    Reader r{tokenize(s)};
    return read_form(r);
}


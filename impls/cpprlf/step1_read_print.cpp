#include <algorithm>
#include <any>
#include <iomanip>
#include <iostream>
#include <ranges>
#include <regex>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

//TODO: Change "EOF" to "end of input" or "unbalanced"
//TODO: Add some const
//TODO: Break this file up as the guide suggests.

// Including <print> caused:
// g++: internal compiler error: File size limit exceeded signal terminated program cc1plus

template <typename... Args>
struct overloaded: Args... { using Args::operator()...; };

using mal_list = std::vector<std::any>;
using mal_type = std::variant<int, std::string, mal_list>;

std::string print_mal_type(const mal_type& t)
{
    std::string print_mal_list(const mal_list& l);
    return std::visit(overloaded{
        [](int v) { return std::to_string(v); },
        [](const std::string& s) { return s; },
        [](const mal_list& l) { return print_mal_list(l); }
    }, t);
}

std::string print_mal_list(const mal_list& l)
{
    std::string result{"("};
    // No std::vector::append_range?
    std::ranges::copy(l
        | std::views::transform([](const auto& e) { return print_mal_type(std::any_cast<mal_type>(e)); })
        | std::views::join_with(' '), std::back_inserter(result));
    result.push_back(')');
    return result;
}

struct reader {
    reader(std::vector<std::string> tokens): my_tokens{std::move(tokens)}, my_position{0} {}

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

    //TODO: Could use source location!
    void throw_eof(const std::string& func) const
    {
        throw std::runtime_error{
            std::string("Unexpected EOF in ") + func + ": " + dump()};
    }

private:
    std::vector<std::string> my_tokens;
    std::size_t my_position;
};

std::vector<std::string> tokenize(std::string_view sv)
{
    std::string s{sv};//TODO remove copy
    std::vector<std::string> tokens;
    std::regex regex{R"RAW([\s,]*(~@|[\[\]{}()'`~^@]|"(?:\\.|[^\\"])*"?|;.*|[^\s\[\]{}('"`,;)]*))RAW"};
    std::sregex_iterator iter(s.begin(), s.end(), regex);
    std::sregex_iterator end;
    for (; iter != end; ++iter) {
        tokens.push_back((*iter)[1].str());
    }
    return tokens;
}

mal_type read_atom(reader& r)
{
    auto token{r.next()};
    if (not token) r.throw_eof(__func__);
    try {
        return std::stoi(*token);
    } catch (const std::invalid_argument&) {
        return *token;
    } 
}

mal_type read_list(reader& r, char closer)
{
    const std::string closer_string(1, closer);
    mal_type read_form(reader& r);
    mal_list list;
    r.next(); // Consume opener
    while (true) {
        auto token{r.peek()};
        if (not token) r.throw_eof(__func__);
        if (closer_string == *token) {
            r.next(); // Consume closer.
            return list;
        }
        list.push_back(read_form(r));
    }
}

mal_type read_form(reader& r)
{
    auto get_closer = [](const auto& token) -> char
    {
        if (token.empty()) return '\0';
        char c{token.at(0)};
        if ('(' == c) return ')';
        if ('[' == c) return ']';
        if ('{' == c) return '}';
        return '\0';
    };

    auto token{r.peek()};
    if (not token) r.throw_eof(__func__);
    char closer{get_closer(*token)};
    if (not ('\0' == closer)) {
        return read_list(r, closer);
    } else {
        return read_atom(r);
    }
}

mal_type read_str(std::string_view s)
{
    reader r{tokenize(s)};
    return read_form(r);
}

auto read(std::string_view s)
{
    return read_str(s);
}

auto eval(auto ast)
{
    return ast;
}

std::string print(mal_type mt)
{
    return print_mal_type(mt);
}

std::string rep(std::string_view s)
{
    return print(eval(read(s)));
}

int main()
{
    while (true) {
        std::cout << "user> ";
        std::string line;
        if (not std::getline(std::cin, line)) break;
        try {
            std::cout << rep(line) << '\n';
        } catch (const std::exception& e) {
            std::cout << "Exception: " << e.what() << '\n';
        }
    }
}

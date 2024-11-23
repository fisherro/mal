#include "printer.hpp"
#include "types.hpp"
#include <algorithm>
#include <any>
#include <iterator>
#include <ranges>
#include <string>
#include <variant>

template <typename... Args>
struct overloaded: Args... { using Args::operator()...; };

std::string format_string(const std::vector<char>& v, bool print_readably)
{
    std::string s;
    if (print_readably) {
        // double-quotes, newlines, and backslashes must be escaped
        // " -> \"
        // newline -> \n
        // backslash -> two backslashes
        //TODO...
        const std::unordered_map<char, std::string> map{
            { '"', "\\\"" },
                { '\n', "\\n" },
                { '\\', "\\\\" },
        };
        s += '"';
        for (char c: v) {
            auto iter{map.find(c)};
            if (map.end() == iter) s.push_back(c);
            else s += iter->second;
        }
        s += '"';
    } else {
        s.assign(v.begin(), v.end());
    }
    return s;
}

std::string pr_str(const mal_type& t, bool print_readably)
{
    // Need to forward declare print_mal_list:
    std::string print_mal_list(const mal_list&, bool);

    return std::visit(overloaded{
        [](int v) { return std::to_string(v); },
        [](const std::string& s) { return s; },
        [print_readably](const std::vector<char>& v)
        { return format_string(v, print_readably); },
        [](mal_nil) -> std::string { return "nil"; },
        [](mal_false) -> std::string { return "false"; },
        [](mal_true) -> std::string { return "true"; },
        [print_readably](const mal_list& l)
        { return print_mal_list(l, print_readably); },
        //TODO: Do we want to distinguish proc & func?
        [](mal_proc p) -> std::string { return "#<function>"; },
        [](mal_func f) -> std::string { return "#<function>"; },
    }, t);
}

std::string print_mal_list(const mal_list& list, bool print_readably)
{
    std::string result{list.get_opener()};
    // No std::vector::append_range?
    std::ranges::copy(mal_list_get(list)
        | std::views::transform([print_readably](const auto& e)
            { return pr_str(e, print_readably); })
        | std::views::join_with(' '), std::back_inserter(result));
    result.push_back(list.get_closer());
    return result;
}


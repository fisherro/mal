#include "printer.hpp"
#include <algorithm>
#include <any>
#include <iterator>
#include <ranges>
#include <string>
#include <variant>

template <typename... Args>
struct overloaded: Args... { using Args::operator()...; };

std::string pr_str(const mal_type& t)
{
    std::string print_mal_list(const mal_list& l);
    return std::visit(overloaded{
        [](int v) { return std::to_string(v); },
        [](const std::string& s) { return s; },
        [](const mal_list& l) { return print_mal_list(l); },
        [](mal_proc p) -> std::string { return "<proc>"; },
    }, t);
}

std::string print_mal_list(const mal_list& l)
{
    std::string result{"("};
    // No std::vector::append_range?
    std::ranges::copy(l
        | std::views::transform([](const auto& e)
            { return pr_str(mal_cast(e)); })
        | std::views::join_with(' '), std::back_inserter(result));
    result.push_back(')');
    return result;
}


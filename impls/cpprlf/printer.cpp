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

std::string pr_str(const mal_type& t)
{
    std::string print_mal_list(const mal_list& l);
    return std::visit(overloaded{
        [](int v) { return std::to_string(v); },
        [](const std::string& s) { return s; },
        [](mal_nil) -> std::string { return "nil"; },
        [](mal_false) -> std::string { return "false"; },
        [](mal_true) -> std::string { return "true"; },
        [](const mal_list& l) { return print_mal_list(l); },
        //TODO: Do we want to distinguish proc & func?
        [](mal_proc p) -> std::string { return "#<function>"; },
        [](mal_func f) -> std::string { return "#<function>"; },
    }, t);
}

std::string print_mal_list(const mal_list& list)
{
    std::string result{list.get_opener()};
    // No std::vector::append_range?
    std::ranges::copy(mal_list_get(list)
        | std::views::transform([](const auto& e)
            { return pr_str(e); })
        | std::views::join_with(' '), std::back_inserter(result));
    result.push_back(list.get_closer());
    return result;
}


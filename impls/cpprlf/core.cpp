#include "env.hpp"
#include "printer.hpp"
#include "reader.hpp"
#include "types.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <ranges>
#include <utility>

#define ADD_INT_OP(ENV, OP) ENV->set(std::string{#OP}, mal_proc{[](const mal_list& args)->mal_type{ return int(args.at_to<int>(0) OP args.at_to<int>(1)); }})

#define ADD_INT_COMP(ENV, OP) ENV->set(std::string{#OP}, mal_proc{[](const mal_list& args)->mal_type{ return bool_it(args.at_to<int>(0) OP args.at_to<int>(1)); }})

#define ADD_FUNC(ENV, FUNC) ENV->set(std::string{#FUNC}, mal_proc{FUNC})

std::string pr_str_true(const mal_type& m) { return pr_str(m, true); }
std::string pr_str_false(const mal_type& m) { return pr_str(m, false); }

mal_type bool_it(bool b)
{
    if (b) return mal_true{};
    return mal_false{};
}

mal_type core_pr_str(const mal_list& args)
{
    return mal_list_get(args)
        | std::views::transform(pr_str_true)
        | std::views::join_with(' ')
        | std::ranges::to<std::vector<char>>();
}

mal_type str(const mal_list& args)
{
    return mal_list_get(args)
        | std::views::transform(pr_str_false)
        | std::views::join
        | std::ranges::to<std::vector<char>>();
}

mal_type prn(const mal_list& args)
{
    std::cout << (mal_list_get(args)
        | std::views::transform(pr_str_true)
        | std::views::join_with(' ')
        | std::ranges::to<std::string>())
        << '\n';
    return mal_nil{};
}

mal_type println(const mal_list& args)
{
    std::cout << (mal_list_get(args)
        | std::views::transform(pr_str_false)
        | std::views::join_with(' ')
        | std::ranges::to<std::string>())
        << '\n';
    return mal_nil{};
}

mal_type list(const mal_list& args)
{
    return args;
}

mal_type is_list(const mal_list& args)
{
    auto list_opt{try_mal_to<mal_list>(mal_list_at(args, 0))};
    if (not list_opt) return mal_false{};
    return bool_it(list_opt->is_list());
}

mal_type is_empty(const mal_list& args)
{
    return bool_it(args.at_to<mal_list>(0).empty());
}

mal_type count(const mal_list& args)
{
    mal_type arg{mal_list_at(args, 0)};
    if (mal_is<mal_nil>(arg)) return 0;
    //TODO: Check for integer overflow?
    return int(mal_to<mal_list>(arg).size());
}

mal_type equals(const mal_list& args)
{
    mal_type lhs{mal_list_at(args, 0)};
    mal_type rhs{mal_list_at(args, 1)};
    return bool_it(lhs == rhs);
}

mal_type core_read_string(const mal_list& args)
{
    return read_str(std::string_view{args.at_to<std::vector<char>>(0)});
}

mal_type slurp(const mal_list& args)
{
    std::ifstream in{
        args.at_to<std::vector<char>>(0)
            | std::ranges::to<std::string>()};
    return std::vector<char>(
            std::istreambuf_iterator<char>(in),
            std::istreambuf_iterator<char>());
}

std::shared_ptr<env> get_ns()
{
    auto ns{env::make()};
    ADD_INT_OP(ns, +);
    ADD_INT_OP(ns, -);
    ADD_INT_OP(ns, *);
    ADD_INT_OP(ns, /);
    ns->set("pr-str", mal_proc{core_pr_str});
    ADD_FUNC(ns, str);
    ADD_FUNC(ns, prn);
    ADD_FUNC(ns, println);
    ADD_FUNC(ns, list);
    ns->set("list?", mal_proc{is_list});
    ns->set("empty?", mal_proc{is_empty});
    ADD_FUNC(ns, count);
    ns->set("=", mal_proc{equals});
    ADD_INT_COMP(ns, <);
    ADD_INT_COMP(ns, <=);
    ADD_INT_COMP(ns, >);
    ADD_INT_COMP(ns, >=);
    ns->set("read-string", mal_proc{core_read_string});
    ADD_FUNC(ns, slurp);
    return ns;
}


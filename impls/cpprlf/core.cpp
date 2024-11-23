#include "env.hpp"
#include "printer.hpp"
#include "types.hpp"
#include <iostream>
#include <utility>

#define ADD_INT_OP(ENV, OP) ENV->set(std::string{#OP}, mal_proc{[](const mal_list& args)->mal_type{ return int(args.at_to<int>(0) OP args.at_to<int>(1)); }})

#define ADD_INT_COMP(ENV, OP) ENV->set(std::string{#OP}, mal_proc{[](const mal_list& args)->mal_type{ return bool_it(args.at_to<int>(0) OP args.at_to<int>(1)); }})

#define ADD_FUNC(ENV, FUNC) ENV->set(std::string{#FUNC}, mal_proc{FUNC})

mal_type bool_it(bool b)
{
    if (b) return mal_true{};
    return mal_false{};
}

mal_type prn(const mal_list& args)
{
    auto argv{mal_list_get(args)};
    bool first{true};
    for (auto& arg: argv) {
        if (not std::exchange(first, false)) std::cout << ' ';
        std::cout << pr_str(arg, true);
    }
    std::cout << '\n';
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

std::shared_ptr<env> get_ns()
{
    auto ns{env::make()};
    ADD_INT_OP(ns, +);
    ADD_INT_OP(ns, -);
    ADD_INT_OP(ns, *);
    ADD_INT_OP(ns, /);
    ADD_FUNC(ns, prn);
    ADD_FUNC(ns, list);
    ns->set("list?", mal_proc{is_list});
    ns->set("empty?", mal_proc{is_empty});
    ADD_FUNC(ns, count);
    ns->set("=", mal_proc{equals});
    ADD_INT_COMP(ns, <);
    ADD_INT_COMP(ns, <=);
    ADD_INT_COMP(ns, >);
    ADD_INT_COMP(ns, >=);
    return ns;
}


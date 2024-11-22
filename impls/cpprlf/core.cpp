#include "env.hpp"
#include "printer.hpp"
#include "types.hpp"
#include <iostream>

#define ADD_INT_OP(ENV, OP) ENV->set(std::string{#OP}, mal_proc{[](const mal_list& args)->mal_type{ return int(mal_list_at_to<int>(args, 0) OP mal_list_at_to<int>(args, 1)); }})

#define ADD_INT_COMP(ENV, OP) ENV->set(std::string{#OP}, mal_proc{[](const mal_list& args)->mal_type{ return bool_it(mal_list_at_to<int>(args, 0) OP mal_list_at_to<int>(args, 1)); }})

#define ADD_FUNC(ENV, FUNC) ENV->set(std::string{#FUNC}, mal_proc{FUNC})

mal_type bool_it(bool b)
{
    if (b) return mal_true{};
    return mal_false{};
}

mal_type prn(const mal_list& args)
{
    //TODO: Pass print_readably as true.
    if (not mal_list_empty(args)) {
        std::cout << pr_str(mal_list_at(args, 0));
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
    return bool_it(mal_list_empty(mal_list_at_to<mal_list>(args, 0)));
}

mal_type count(const mal_list& args)
{
    mal_type arg{mal_list_at(args, 0)};
    if (mal_is<mal_nil>(arg)) return 0;
    //TODO: Check for integer overflow?
    return int(mal_list_size(mal_to<mal_list>(arg)));
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


#include "env.hpp"
#include "printer.hpp"
#include "types.hpp"
#include <iostream>

#define ADD_INT_OP(ENV, OP) ENV.set(std::string{#OP}, mal_proc{[](const mal_list& args)->mal_type{ return int(mal_list_at_to<int>(args, 0) OP mal_list_at_to<int>(args, 1)); }})

mal_type prn(const mal_list& args)
{
    //TODO: Pass print_readably as true.
    if (not mal_list_empty(args)) {
        std::cout << pr_str(mal_list_at(args, 0));
    }
    std::cout << '\n';
    return mal_nil{};
}

env get_ns()
{
    env ns;
    ADD_INT_OP(ns, +);
    ADD_INT_OP(ns, -);
    ADD_INT_OP(ns, *);
    ADD_INT_OP(ns, /);
    ns.set("prn", mal_proc{prn});
    return ns;
}


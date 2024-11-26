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

mal_type make_atom(const mal_list& args)
{
    return std::make_shared<atom>(mal_list_at(args, 0));
}

mal_type is_atom(const mal_list& args)
{
    auto atom_opt{try_mal_to<std::shared_ptr<atom>>(mal_list_at(args, 0))};
    return bool_it(atom_opt.has_value());
}

mal_type deref(const mal_list& args)
{
    return args.at_to<std::shared_ptr<atom>>(0)->deref();
}

mal_type reset(const mal_list& args)
{
    auto atom_ptr{args.at_to<std::shared_ptr<atom>>(0)};
    auto value{mal_list_at(args, 1)};
    atom_ptr->reset(value);
    return value;
}

// Probably belongs in types.hpp/.cpp
mal_proc get_proc(mal_type m)
{
    if (auto func_ptr{std::get_if<mal_func>(&m)}; func_ptr) {
        return func_ptr->proc();
    }
    if (auto proc_ptr{std::get_if<mal_proc>(&m)}; proc_ptr) {
        return *proc_ptr;
    } else {
        //TODO: Unify the use of exceptions
        throw std::runtime_error{"That's not invocable!"};
    }
}

mal_type atom_swap(const mal_list& args)
{
    // If this were Clojure instead of mal, this should be mutex protected.
    auto argv{mal_list_get(args)};
    auto atom_ptr{mal_to<std::shared_ptr<atom>>(argv.at(0))};
    auto proc{get_proc(argv.at(1))};

    mal_list new_args;
    mal_list_add(new_args, atom_ptr->deref());
    std::ranges::subrange rest{argv.begin() + 2, argv.end()};
    for (auto& element: rest) {
        mal_list_add(new_args, element);
    }

    mal_type result{mal_proc_call(proc, new_args)};
    atom_ptr->reset(result);
    return result;
}

mal_type cons(const mal_list& args)
{
    mal_type head{mal_list_at(args, 0)};
    std::vector<mal_type> rest{mal_list_get(args.at_to<mal_list>(1))};
    mal_list result;
    mal_list_add(result, head);
    for (const mal_type& element: rest) mal_list_add(result, element);
    return result;
}

mal_type concat(const mal_list& args)
{
    auto argv{mal_list_get(args)};
    mal_list result;
    for (const auto& arg: argv) {
        auto vec{mal_list_get(mal_to<mal_list>(arg))};
        for (const auto& element: vec) {
            mal_list_add(result, element);
        }
    }
    return result;
}

mal_type vec(const mal_list& args)
{
    auto arg{args.at_to<mal_list>(0)};
    if (arg.is_vector()) return arg;
    mal_list v{'['};
    auto elements{mal_list_get(arg)};
    for (const auto& element: elements) {
        mal_list_add(v, element);
    }
    return v;
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
    ns->set("atom", mal_proc{make_atom});
    ns->set("atom?", mal_proc{is_atom});
    ADD_FUNC(ns, deref);
    ns->set("reset!", mal_proc{reset});
    ns->set("swap!", mal_proc{atom_swap});
    ADD_FUNC(ns, cons);
    ADD_FUNC(ns, concat);
    ADD_FUNC(ns, vec);
    return ns;
}


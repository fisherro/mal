#include "env.hpp"
#include "printer.hpp"
#include "reader.hpp"
#include "types.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <ranges>
#include <span>
#include <utility>

//TODO: See if there's anywhere to use...
//      std::optional::value_or
//                     and_then
//                     transform
//                     or_else

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

// Creates a "type?" predicate and adds it to the given environment.
template <typename T>
void make_is_type(std::shared_ptr<env> an_env, std::string name)
{
    auto p = [=](const mal_list& args)
    {
        auto opt{try_mal_to<T>(mal_list_at(args, 0))};
        return bool_it(opt.has_value());
    };
    an_env->set(name, mal_proc{p});
}

///////////////////////////////////////////////////////////////////////////////

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
    //TODO: Fails for maps
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
    if (not arg.is_vector()) arg.become_vector();
    return arg;
}

mal_type nth(const mal_list& args)
{
    auto list{args.at_to<mal_list>(0)};
    auto n{args.at_to<int>(1)};
    if (n >= int(list.size())) {
        // The tests didn't like the text given by the standard exception.
        throw std::runtime_error{
            "nth out-of-bounds index; n = "
            + std::to_string(n)
            + "; bounds = "
            + std::to_string(list.size())};
    }
    return mal_list_at(list, n);
}

mal_type first(const mal_list& args)
{
    auto maybe_list{mal_list_at(args, 0)};
    if (auto nil_ptr{get_if<mal_nil>(&maybe_list)}; nil_ptr) return maybe_list;
    auto list{mal_to<mal_list>(maybe_list)};
    if (list.empty()) return mal_nil{};
    return mal_list_at(list, 0);
}

mal_type rest(const mal_list& args)
{
    auto maybe_list{mal_list_at(args, 0)};
    if (auto nil_ptr{get_if<mal_nil>(&maybe_list)}; nil_ptr) return mal_list{};
    auto list{mal_to<mal_list>(maybe_list)};
    if (list.empty()) {
        // Even when passed a vector, we return a list.
        list.become_list();
        return list;
    }
    auto result{list.rest()};
    // Even when passed a vector, we return a list.
    result.become_list();
    return result;
}

mal_type is_macro(const mal_list& args)
{
    auto func_opt{try_mal_to<mal_func>(mal_list_at(args, 0))};
    if (not func_opt) return mal_false{};
    return bool_it(func_opt->is_macro());
}

mal_type mal_throw(const mal_list& args)
{
#if 0
    // Calls terminate
    throw mal_list_at(args, 0);
#endif
#if 1
    throw std::runtime_error{pr_str(mal_list_at(args, 0), false)};
#endif
#if 0
    // Calls terminate
    throw mal_exception{args};
#endif
}

mal_type apply(const mal_list& args)
{
    auto argv{mal_list_get(args)};
    mal_proc p{[](const mal_list& args){ return mal_nil{}; }};
    auto fptr{std::get_if<mal_func>(&argv.front())};
    if (fptr) p = fptr->proc();
    else p = std::get<mal_proc>(argv.front());
    mal_list apply_args;
    std::ranges::subrange middle{argv.begin() + 1, argv.end() - 1};
    for (auto arg: middle) {
        //TODO: Need to evaluate args?
        mal_list_add(apply_args, arg);
    }
    auto rest{mal_list_get(std::get<mal_list>(argv.back()))};
    for (auto arg: rest) {
        mal_list_add(apply_args, arg);
    }
    return mal_proc_call(p, apply_args);
}

mal_type map(const mal_list& args)
{
    //TODO: Could refactor out getting the mal_proc from apply and here.
    auto argv{mal_list_get(args)};
    mal_proc p{[](const mal_list& args){ return mal_nil{}; }};
    auto fptr{std::get_if<mal_func>(&argv.front())};
    if (fptr) p = fptr->proc();
    else p = std::get<mal_proc>(argv.front());
    auto v{mal_list_get(mal_to<mal_list>(argv.at(1)))};
    mal_list results;
    for (auto& element: v) {
        mal_list list;
        mal_list_add(list, element);
        mal_list_add(results, mal_proc_call(p, list));
    }
    return results;
}

mal_type is_symbol(const mal_list& args)
{
    auto opt{try_mal_to<std::string>(mal_list_at(args, 0))};
    if (not opt) return mal_false{};
    return bool_it(not is_keyword(*opt));
}

mal_type symbol(const mal_list& args)
{
    return args.at_to<std::vector<char>>(0) | std::ranges::to<std::string>();
}

mal_type keyword(const mal_list& args)
{
    auto arg{mal_list_at(args, 0)};
    if (auto kw_ptr{std::get_if<std::string>(&arg)}; kw_ptr) {
        if (is_keyword(*kw_ptr)) return arg;
        else throw std::runtime_error{"arg is a symbol"};
    }
    if (auto v_ptr{std::get_if<std::vector<char>>(&arg)}; v_ptr) {
        return std::string(1, ':') + (*v_ptr | std::ranges::to<std::string>());
    }
    throw std::runtime_error{"arg to keyword is not a string"};
}

mal_type core_is_keyword(const mal_list& args)
{
    auto opt{try_mal_to<std::string>(mal_list_at(args, 0))};
    if (not opt) return mal_false{};
    return bool_it(is_keyword(*opt));
}

mal_type vector(const mal_list& args)
{
    mal_list copy{args};
    copy.become_vector();
    return copy;
}

mal_type is_vector(const mal_list& args)
{
    auto list_opt{try_mal_to<mal_list>(mal_list_at(args, 0))};
    if (not list_opt) return mal_false{};
    return bool_it(list_opt->is_vector());
}

mal_type is_sequential(const mal_list& args)
{
    auto list_opt{try_mal_to<mal_list>(mal_list_at(args, 0))};
    if (not list_opt) return mal_false{};
    return bool_it(list_opt->is_list() or list_opt->is_vector());
}

mal_type hash_map(const mal_list& args)
{
    mal_map result;
    if (0 == args.size()) return result;
    if (0 != (args.size() % 2)) throw std::runtime_error{"missing map value"};
    auto cpp_vector{mal_list_get(args)};
    //TODO: Look to see if span could replace uses of subrange.
    std::span<mal_type> span(cpp_vector);
    while (span.size() > 0) {
        mal_type outer_key{span[0]};
        mal_type value{span[1]};
        mal_map_set(result, outer_key, value);
        span = span.subspan(2);
    }
    return result;
}

mal_type is_map(const mal_list& args)
{
    auto arg{mal_list_at(args, 0)};
    if (auto list_ptr{std::get_if<mal_list>(&arg)}; list_ptr) {
        return bool_it(list_ptr->is_map());
    }
    if (auto map_ptr{std::get_if<mal_map>(&arg)}; map_ptr) {
        return mal_true{};
    }
    return mal_false{};
}

mal_type assoc(const mal_list& args)
{
    auto argv{mal_list_get(args)};
    auto map{mal_to<mal_map>(argv.at(0))};
    auto the_rest{std::span<mal_type>(argv).subspan(1)};
    if (0 == the_rest.size()) return map;
    if (0 != (the_rest.size() % 2)) {
        throw std::runtime_error{"missing map value"};
    }
    while (the_rest.size() > 0) {
        mal_type outer_key{the_rest[0]};
        mal_type value{the_rest[1]};
        mal_map_set(map, outer_key, value);
        the_rest = the_rest.subspan(2);
    }
    return map;
}

mal_type dissoc(const mal_list& args)
{
    auto argv{mal_list_get(args)};
    auto map{mal_to<mal_map>(argv.at(0))};
    auto the_rest{std::span<mal_type>(argv).subspan(1)};
    if (0 == the_rest.size()) return map;
    for (auto& key: the_rest) {
        mal_map_remove(map, key);
    }
    return map;
}

mal_type map_get(const mal_list& args)
{
    try {
        auto map{args.at_to<mal_map>(0)};
        auto key{mal_list_at(args, 1)};
        auto value_opt{mal_map_get(map, key)};
        if (not value_opt) return mal_nil{};
        return *value_opt;
    } catch (...) {
        return mal_nil{};
    }
}

mal_type contains(const mal_list& args)
{
    auto map{args.at_to<mal_map>(0)};
    auto key{mal_list_at(args, 1)};
    auto value_opt{mal_map_get(map, key)};
    return bool_it(value_opt.has_value());
}

mal_type keys(const mal_list& args)
{
    auto map{args.at_to<mal_map>(0)};
    auto pairs{mal_map_pairs(map)};
    // Theres a view for this...but I'm not using it.
    mal_list results;
    for (auto& pair: pairs) {
        mal_list_add(results, mal_map_ikey_to_okey(pair.first));
    }
    return results;
}

mal_type vals(const mal_list& args)
{
    auto map{args.at_to<mal_map>(0)};
    auto pairs{mal_map_pairs(map)};
    // Theres a view for this...but I'm not using it.
    mal_list results;
    for (auto& pair: pairs) mal_list_add(results, pair.second);
    return results;
}

mal_type core_readline(const mal_list& args)
{
    auto prompt{args.at_to<std::vector<char>>(0)};
    std::cout << (prompt | std::ranges::to<std::string>());
    std::string line;
    if (not std::getline(std::cin, line)) return mal_nil{};
    return line | std::ranges::to<std::vector<char>>();
}

mal_type not_implemented(const mal_list&)
{
    throw std::runtime_error("not implemented");
}

mal_type is_callable(const mal_list& args)
{
    mal_type arg{mal_list_at(args, 0)};
    auto fptr{std::get_if<mal_func>(&arg)};
    auto pptr{std::get_if<mal_proc>(&arg)};
    return bool_it((fptr and (not fptr->is_macro())) or pptr);
}

///////////////////////////////////////////////////////////////////////////////

std::shared_ptr<env> get_ns()
{
    //TODO: Make a macro to handle adding the is_x -> x? predicates.
    //TODO: Make a macro for _ -> -
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
    ADD_FUNC(ns, nth);
    ADD_FUNC(ns, first);
    ADD_FUNC(ns, rest);
    ns->set("macro?", mal_proc{is_macro});
    ns->set("throw", mal_proc{mal_throw});
    ADD_FUNC(ns, apply);
    ADD_FUNC(ns, map);
    make_is_type<mal_nil>(ns, "nil?");
    make_is_type<mal_true>(ns, "true?");
    make_is_type<mal_false>(ns, "false?");
    ns->set("symbol?", mal_proc{is_symbol});
    ADD_FUNC(ns, symbol);
    ADD_FUNC(ns, keyword);
    ns->set("keyword?", mal_proc{core_is_keyword});
    ADD_FUNC(ns, vector);
    ns->set("vector?", mal_proc{is_vector});
    ns->set("sequential?", mal_proc{is_sequential});
    ns->set("hash-map", mal_proc{hash_map});
    ns->set("map?", mal_proc{is_map});
    ADD_FUNC(ns, assoc);
    ADD_FUNC(ns, dissoc);
    ns->set("get", mal_proc{map_get});
    ns->set("contains?", mal_proc{contains});
    ADD_FUNC(ns, keys);
    ADD_FUNC(ns, vals);
    ns->set("readline", mal_proc{core_readline});
    ns->set("fn?", mal_proc{is_callable});

    std::vector<std::string> missing{
        "time-ms", "meta", "with-meta",
        "string?", "number?", "seq", "conj",};
    for (auto& symbol: missing) {
        ns->set(symbol, mal_proc{not_implemented});
    }

    return ns;
}


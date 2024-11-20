#include "env.hpp"
#include "printer.hpp"
#include "reader.hpp"
#include <algorithm>
#include <exception>
#include <functional>
#include <iostream>
#include <ranges>
#include <string>
#include <string_view>

#define ADD_INT_OP(ENV, OP) ENV.set(std::string{#OP}, mal_proc{[](const mal_list& args)->mal_type{ return int(mal_list_at_to<int>(args, 0) OP mal_list_at_to<int>(args, 1)); }})

auto read(std::string_view s)
{
    return read_str(s);
}

bool eval_debug(env& current_env)
{
    if (not current_env.has("DEBUG-EVAL")) return false;
    mal_type debug_eval{current_env.get("DEBUG-EVAL")};
    if (mal_is<mal_nil>(debug_eval)) return false;
    if (mal_is<mal_false>(debug_eval)) return false;
    return true;
}

mal_type eval(const mal_type& ast, env& current_env)
{
    if (eval_debug(current_env)) {
        std::cout << "EVAL: " << pr_str(ast) << '\n';
    }
    if (auto sp{std::get_if<std::string>(&ast)}; sp) {
        return current_env.get(*sp);
    }
    if (auto list{std::get_if<mal_list>(&ast)}; list) {
        if (mal_list_empty(*list)) return ast;
        const mal_type head{mal_list_at(*list, 0)};
        if (auto symbol{std::get_if<std::string>(&head)}; symbol) {
            if (*symbol == "def!") {
                mal_type value{eval(mal_list_at(*list, 2), current_env)};
                current_env.set(mal_list_at_to<std::string>(*list, 1), value);
                return value;
            }
            if (*symbol == "let*") {
                mal_list args{mal_list_at_to<mal_list>(*list, 1)};
                auto argsv{mal_list_get(args)};
                std::ranges::reverse(argsv);
                env new_env{current_env};
                while (not argsv.empty()) {
                    std::string key{mal_to<std::string>(argsv.back())};
                    argsv.pop_back();
                    mal_type value_form{argsv.back()};
                    argsv.pop_back();
                    mal_type value{eval(value_form, new_env)};
                    new_env.set(key, value);
                }
                mal_type body{mal_list_at(*list, 2)};
                return eval(body, new_env);
            }
            if (*symbol == "do") {
                auto vector{mal_list_get(*list)};
                std::ranges::subrange rest{vector.begin() + 1, vector.end()};
                mal_type result;
                for (const auto& element: rest) {
                    result = eval(element, current_env);
                }
                return result;
            }
        }
        // A mal_proc takes a mal_list and returns an std::any.
        const mal_type proc_box{eval(head, current_env)};
        if (auto proc_ptr{std::get_if<mal_proc>(&proc_box)}; proc_ptr) {
            mal_list evaluated_args;
            auto vector{mal_list_get(*list)};
            std::ranges::subrange rest{vector.begin() + 1, vector.end()};
            for (auto& arg: rest) {
                mal_type evaluated{eval(arg, current_env)};
                mal_list_add(evaluated_args, evaluated);
            }
            return mal_proc_call(*proc_ptr, evaluated_args);
        }
    }
    return ast;
}

std::string print(const mal_type& mt)
{
    return pr_str(mt);
}

std::string rep(std::string_view s, env& env)
{
    return print(eval(read(s), env));
}

//TODO: Move to core.cpp.
mal_type prn(const mal_list& args)
{
    //TODO: Pass print_readably as true.
    if (not mal_list_empty(args)) {
        std::cout << pr_str(mal_list_at(args, 0));
    }
    std::cout << '\n';
    return mal_nil{};
}

int main()
{
    env repl_env;
    ADD_INT_OP(repl_env, +);
    ADD_INT_OP(repl_env, -);
    ADD_INT_OP(repl_env, *);
    ADD_INT_OP(repl_env, /);
    repl_env.set("prn", mal_proc{prn});

    while (true) {
        std::cout << "user> ";
        std::string line;
        if (not std::getline(std::cin, line)) break;
        try {
            std::cout << rep(line, repl_env) << '\n';
        } catch (const std::exception& e) {
            std::cout << "Exception: " << e.what() << '\n';
        }
    }
}


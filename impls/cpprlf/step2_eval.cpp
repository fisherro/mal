#include "printer.hpp"
#include "reader.hpp"
#include <exception>
#include <functional>
#include <iostream>
#include <ranges>
#include <string>
#include <string_view>

// Including <print> caused:
// g++: internal compiler error: File size limit exceeded signal terminated program cc1plus

#define MAKE_INT_OP(OP) std::make_pair(std::string{#OP}, mal_proc{[](const mal_list& args)->mal_type{ return int(mal_list_at_to<int>(args, 0) OP mal_list_at_to<int>(args, 1)); }})

using mal_env = std::unordered_map<std::string, mal_proc>;

mal_env repl_env {
    MAKE_INT_OP(+),
    MAKE_INT_OP(-),
    MAKE_INT_OP(*),
    MAKE_INT_OP(/),
};

auto read(std::string_view s)
{
    return read_str(s);
}

mal_type eval(const auto& ast, const mal_env& env)
{
    //TODO: Use visit?
    if (auto sp{std::get_if<std::string>(&ast)}; sp) {
        auto iter{env.find(*sp)};
        if (env.end() != iter) return iter->second;
        std::string message{"Symbol "};
        message += *sp;
        message += " not found";
        throw std::runtime_error{message};
    }
    // A mal_proc takes a mal_list and returns an std::any.
    if (auto list{std::get_if<mal_list>(&ast)}; list) {
        if (mal_list_empty(*list)) return ast;
        const mal_type symbol{mal_list_at(*list, 0)};
        const mal_type proc_box{eval(symbol, env)};
        if (auto proc_ptr{std::get_if<mal_proc>(&proc_box)}; proc_ptr) {
            mal_list evaluated_args;
            auto vector{mal_list_get(*list)};
            std::ranges::subrange rest{vector.begin() + 1, vector.end()};
            for (auto& arg: rest) {
                mal_type evaluated{eval(arg, env)};
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

std::string rep(std::string_view s)
{
    return print(eval(read(s), repl_env));
}

int main()
{
    while (true) {
        std::cout << "user> ";
        std::string line;
        if (not std::getline(std::cin, line)) break;
        try {
            std::cout << rep(line) << '\n';
        } catch (const std::exception& e) {
            std::cout << "Exception: " << e.what() << '\n';
        }
    }
}


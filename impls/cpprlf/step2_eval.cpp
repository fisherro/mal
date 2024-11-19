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

int to_int(const std::any& m)
{
    return std::get<int>(mal_cast(m));
}

using mal_env = std::unordered_map<std::string, mal_proc>;
mal_env repl_env {
    { "+", [](const mal_list args) -> mal_type
        { return to_int(args.at(0)) + to_int(args.at(1)); } },
    { "-", [](const mal_list args) -> mal_type
        { return to_int(args.at(0)) - to_int(args.at(1)); } },
    { "*", [](const mal_list args) -> mal_type
        { return to_int(args.at(0)) * to_int(args.at(1)); } },
    { "/", [](const mal_list args) -> mal_type
        { return int(to_int(args.at(0)) / to_int(args.at(1))); } },
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
        if (list->empty()) return ast;
        const mal_type symbol{mal_cast(list->front())};
        const mal_type proc_box{eval(symbol, env)};
        if (auto proc_ptr{std::get_if<mal_proc>(&proc_box)}; proc_ptr) {
            mal_list evaluated_args;
            std::ranges::subrange rest{list->begin() + 1, list->end()};
            for (auto& arg: rest) {
                mal_type evaluated{eval(mal_cast(arg), env)};
                evaluated_args.push_back(evaluated);
            }
            return mal_cast((*proc_ptr)(evaluated_args));
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


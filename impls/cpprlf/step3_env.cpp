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

// The mal_cast function is redundant with mal_to<mal_type>.
template <typename type>
type mal_to(const std::any& m)
{
    return std::get<type>(mal_cast(m));
}

auto read(std::string_view s)
{
    return read_str(s);
}

//TODO Make ast mal_type instead of auto.
mal_type eval(const auto& ast, env& current_env)
{
#if 0
    //TODO Gate this with DEBUG-EVAL being in the environment
    std::cout << "EVAL:" << pr_str(ast) << '\n';
#endif
    //TODO: Use visit?
    if (auto sp{std::get_if<std::string>(&ast)}; sp) {
        return current_env.get(*sp);
    }
    // A mal_proc takes a mal_list and returns an std::any.
    if (auto list{std::get_if<mal_list>(&ast)}; list) {
        if (list->empty()) return ast;
        const mal_type head{mal_cast(list->front())};
        if (auto symbol{std::get_if<std::string>(&head)}; symbol) {
            if (*symbol == "def!") {
                mal_type value{eval(mal_cast(list->at(2)), current_env)};
                current_env.set(mal_to<std::string>(list->at(1)), value);
                return value;
            }
            if (*symbol == "let*") {
                mal_list args{mal_to<mal_list>(list->at(1))};
                std::ranges::reverse(args);
                env new_env{current_env};
                while (not args.empty()) {
                    //TODO: bad_any_cast here:
                    std::string key{mal_to<std::string>(args.back())};
                    args.pop_back();
                    mal_type value_form{mal_cast(args.back())};
                    args.pop_back();
                    mal_type value{eval(value_form, new_env)};
                    new_env.set(key, value);
                }
                mal_type body{mal_cast(list->at(2))};
                return eval(body, new_env);
            }
        }
        const mal_type proc_box{eval(head, current_env)};
        if (auto proc_ptr{std::get_if<mal_proc>(&proc_box)}; proc_ptr) {
            mal_list evaluated_args;
            std::ranges::subrange rest{list->begin() + 1, list->end()};
            for (auto& arg: rest) {
                mal_type evaluated{eval(mal_cast(arg), current_env)};
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

std::string rep(std::string_view s, env& env)
{
    return print(eval(read(s), env));
}

int main()
{
    //TODO: We may need to move this to main.
    env repl_env;
    repl_env.set("+", [](const mal_list args) -> mal_type
        { return mal_to<int>(args.at(0)) + mal_to<int>(args.at(1)); });
    repl_env.set("-", [](const mal_list args) -> mal_type
        { return mal_to<int>(args.at(0)) - mal_to<int>(args.at(1)); });
    repl_env.set("*", [](const mal_list args) -> mal_type
        { return mal_to<int>(args.at(0)) * mal_to<int>(args.at(1)); });
    repl_env.set("/", [](const mal_list args) -> mal_type
        { return int(mal_to<int>(args.at(0)) / mal_to<int>(args.at(1))); });
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


#include "core.hpp"
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

auto read(std::string_view s)
{
    return read_str(s);
}

bool check_debug(std::string symbol, std::shared_ptr<env> current_env)
{
    if (not current_env->has(symbol)) return false;
    mal_type debug_eval{current_env->get(symbol)};
    return mal_truthy(debug_eval);
}

//Note: We passed the "too much recursion" test before implementing TCO.
//      Presumably gcc did TCO for us.
mal_type eval(mal_type ast, std::shared_ptr<env> current_env)
{
    //TODO: Can't rebind ast for TCO. What to do?
    //      For now let's just change it to a value.
    while (true) {
        if (check_debug("DEBUG-EVAL", current_env)) {
            std::cout << "EVAL: " << pr_str(ast) << '\n';
            if (check_debug("DEBUG-EVAL-ENV", current_env)) {
                current_env->dump(std::cout);
            }
        }
        if (auto sp{std::get_if<std::string>(&ast)}; sp) {
            return current_env->get(*sp);
        }
        if (auto list{std::get_if<mal_list>(&ast)}; list) {
            if (mal_list_empty(*list)) return ast;
            const mal_type head{mal_list_at(*list, 0)};
            if (auto symbol{std::get_if<std::string>(&head)}; symbol) {
                if (*symbol == "def!") {
                    mal_type value{eval(mal_list_at(*list, 2), current_env)};
                    current_env->set(mal_list_at_to<std::string>(*list, 1), value);
                    return value;
                }
                if (*symbol == "let*") {
                    mal_list args{mal_list_at_to<mal_list>(*list, 1)};
                    auto argsv{mal_list_get(args)};
                    std::ranges::reverse(argsv);
                    auto new_env{env::make(current_env)};
                    while (not argsv.empty()) {
                        std::string key{mal_to<std::string>(argsv.back())};
                        argsv.pop_back();
                        mal_type value_form{argsv.back()};
                        argsv.pop_back();
                        mal_type value{eval(value_form, new_env)};
                        new_env->set(key, value);
                    }
                    mal_type body{mal_list_at(*list, 2)};
                    // TCO:
                    ast = body;
                    current_env = new_env;
                    continue;
                }
                if (*symbol == "do") {
                    auto vector{mal_list_get(*list)};
                    if (vector.size() < 2) {
                        throw std::runtime_error{"empty 'do'!"};
                    }
                    if (vector.size() > 2) {
                        std::ranges::subrange rest{
                            vector.begin() + 1, vector.end() - 1};
                        for (const auto& element: rest) {
                            eval(element, current_env);
                        }
                    }
                    // TCO:
                    ast = vector.back();
                    continue;
                }
                if (*symbol == "if") {
                    mal_type condition{mal_list_at(*list, 1)};
                    if (mal_truthy(eval(condition, current_env))) {
                        // TCO:
                        ast = mal_list_at(*list, 2);
                        continue;
                    } else {
                        std::optional<mal_type> alternate{
                            try_mal_list_at(*list, 3)};
                        if (not alternate) return mal_nil{};
                        // TCO:
                        ast = *alternate;
                        continue;
                    }
                }
                if (*symbol == "fn*") {
                    mal_list binds{mal_list_at_to<mal_list>(*list, 1)};
                    mal_type body{mal_list_at(*list, 2)};
                    auto closure = [=](const mal_list& args) -> mal_type
                    {
                        auto new_env{env::make(current_env)};
                        // The process says that adding the binds/args to the
                        // environment should be done in the environment's ctor.
                        auto binds_vector{mal_list_get(binds)};
                        auto args_vector{mal_list_get(args)};
                        auto arg_iter{args_vector.begin()};
                        for (auto& bind: binds_vector) {
                            if (arg_iter == args_vector.end()) break;
                            new_env->set(mal_to<std::string>(bind), *arg_iter);
                            ++arg_iter;
                        }
                        return eval(body, new_env);
                    };
                    return mal_proc{closure};
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
}

std::string print(const mal_type& mt)
{
    return pr_str(mt);
}

std::string rep(std::string_view s, std::shared_ptr<env> current_env)
{
    return print(eval(read(s), current_env));
}

int main()
{
    try {
        auto repl_env{env::make(get_ns())};
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
    } catch (const std::exception& e) {
        std::cout << "\nEXCEPTION: " << e.what() << '\n';
    }
}


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

mal_type eval(mal_type ast, std::shared_ptr<env> current_env)
{
    if (check_debug("DEBUG-EVAL", current_env)) {
        std::cout << "EVAL: " << pr_str(ast, true) << '\n';
        if (check_debug("DEBUG-EVAL-ENV", current_env)) {
            current_env->dump(std::cout);
        }
    }
    if (auto sp{std::get_if<std::string>(&ast)}; sp) {
        if (is_keyword(*sp)) return ast;
        return current_env->get(*sp);
    }
    if (auto list{std::get_if<mal_list>(&ast)}; list) {
        if (not list->is_list()) {
            mal_list results{list->get_opener()};
            auto cpp_vector{mal_list_get(*list)};
            for (auto element: cpp_vector) {
                mal_list_add(results, eval(element, current_env));
            }
            return results;
        }
        if (list->empty()) return ast;
        const mal_type head{mal_list_at(*list, 0)};
        if (auto symbol{std::get_if<std::string>(&head)}; symbol) {
            if (*symbol == "def!") {
                mal_type value{eval(mal_list_at(*list, 2), current_env)};
                current_env->set(list->at_to<std::string>(1), value);
                return value;
            }
            if (*symbol == "let*") {
                mal_list args{list->at_to<mal_list>(1)};
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
            if (*symbol == "if") {
                mal_type condition{mal_list_at(*list, 1)};
                if (mal_truthy(eval(condition, current_env))) {
                    mal_type consequent{mal_list_at(*list, 2)};
                    return eval(consequent, current_env);
                } else {
                    std::optional<mal_type> alternate{
                        try_mal_list_at(*list, 3)};
                    if (not alternate) return mal_nil{};
                    return eval(*alternate, current_env);
                }
            }
            if (*symbol == "fn*") {
                // Partial retrofit of the step 5 mal_func code:
                mal_func f{*list, current_env};
                return f.proc();
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
    return pr_str(mt, true);
}

std::string rep(std::string_view s, std::shared_ptr<env> current_env)
{
    return print(eval(read(s), current_env));
}

int main()
{
    try {
        auto repl_env{env::make(get_ns())};
        rep("(def! not (fn* (a) (if a false true)))", repl_env);
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


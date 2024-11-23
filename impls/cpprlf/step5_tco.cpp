#include "core.hpp"
#include "env.hpp"
#include "printer.hpp"
#include "reader.hpp"
#include <algorithm>
#include <exception>
#include <functional>
#include <iostream>
#include <ranges>
#include <span>
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
//      But we'll go ahead and do it explicitly.
mal_type eval(mal_type ast, std::shared_ptr<env> current_env)
{
    while (true) {
        if (check_debug("DEBUG-EVAL", current_env)) {
            std::cout << "EVAL: " << pr_str(ast, true) << '\n';
            if (check_debug("DEBUG-EVAL-ENV", current_env)) {
                current_env->dump(std::cout);
            }
        }
        if (auto sp{std::get_if<std::string>(&ast)}; sp) {
            // Keywords evaluate to themselves.
            // Does a lone colon could as a keyword?
            if ((sp->size() > 0) and (':' == sp->at(0))) return ast;
            return current_env->get(*sp);
        }
        if (auto list{std::get_if<mal_list>(&ast)}; list) {
            if (list->is_map()) {
                // Maps are read as lists. When the list is evaluated, we
                // convert it to an actual mal_map object.
                mal_map result;
                if (0 == list->size()) {
                    return result;
                }
                if (0 != (list->size() % 2)) {
                    throw std::runtime_error{"missing map value"};
                }
                auto cpp_vector{mal_list_get(*list)};
                std::span<mal_type> span(cpp_vector);
                while (span.size() > 0) {
                    mal_type outer_key{eval(span[0], current_env)};
                    mal_type value{eval(span[1], current_env)};
                    mal_map_set(result, outer_key, value);
                    span = span.subspan(2);
                }
                return result;
            }
            if (list->is_vector()) {
                mal_list results{'['};
                auto cpp_vector{mal_list_get(*list)};
                for (auto& element: cpp_vector) {
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
                    return mal_func{*list, current_env};
                }
            }
            const mal_type call_me{eval(head, current_env)};
            mal_list evaluated_args;
            auto vector{mal_list_get(*list)};
            std::ranges::subrange rest{vector.begin() + 1, vector.end()};
            for (auto& arg: rest) {
                mal_type evaluated{eval(arg, current_env)};
                mal_list_add(evaluated_args, evaluated);
            }
            if (auto proc_ptr{std::get_if<mal_proc>(&call_me)}; proc_ptr) {
                return mal_proc_call(*proc_ptr, evaluated_args);
            }
            if (auto func_ptr{std::get_if<mal_func>(&call_me)}; func_ptr) {
                // TCO:
                ast = mal_func_ast(*func_ptr);
                current_env = func_ptr->make_env(evaluated_args);
                continue;
            }
            //TODO: Better exception here?
            throw std::runtime_error{"That's not invocable!"};
        }
        return ast;
    }
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


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

//TODO: Change a bunch of std::string parameters to string_view
bool check_debug(std::string symbol, std::shared_ptr<env> current_env)
{
    if (not current_env->has(symbol)) return false;
    mal_type debug_eval{current_env->get(symbol)};
    return mal_truthy(debug_eval);
}

bool is_head_this_symbol(const mal_list& list, const std::string& symbol)
{
    if (list.empty()) return false;
    auto head_opt{try_mal_to<std::string>(mal_list_at(list, 0))};
    if (not head_opt) return false;
    return symbol == *head_opt;
}

bool is_head_this_symbol(const mal_type& object, const std::string& symbol)
{
    auto list_ptr{std::get_if<mal_list>(&object)};
    if (not list_ptr) return false;
    return is_head_this_symbol(*list_ptr, symbol);
}

bool is_head_this_symbol(
        const std::vector<mal_type>& vec,
        const std::string& symbol)
{
    if (vec.empty()) return false;
    auto head_opt{try_mal_to<std::string>(vec.at(0))};
    if (not head_opt) return false;
    return symbol == *head_opt;
}

mal_list make_list(
        const std::string& head,
        const mal_type& last)
{
    mal_list list;
    mal_list_add(list, head);
    mal_list_add(list, last);
    return list;
}

mal_list make_list(
        const std::string& head,
        const mal_type& second, 
        const mal_type& last)
{
    mal_list list{make_list(head, second)};
    mal_list_add(list, last);
    return list;
}

mal_type quasiquote(mal_type ast)
{
    if (auto list_ptr{std::get_if<mal_list>(&ast)}; list_ptr) {
        if (list_ptr->empty()) {
#ifdef I_DID_IT_MY_WAY
            // This worked, but triggered a test soft-fail.
            // The spec says we should convert the vector to a call to vec.
            return ast;
#else
            if (list_ptr->is_vector()) {
                list_ptr->become_list();
                return make_list("vec", *list_ptr);
            }
            return ast;
#endif
        }
        if (list_ptr->is_map()) {
            return make_list("quote", ast);
        }
        if ((not list_ptr->is_vector())
            and is_head_this_symbol(*list_ptr, "unquote"))
        {
            return mal_list_at(*list_ptr, 1);
        }
        auto vec{mal_list_get(*list_ptr)};
        mal_type result{mal_list{}};
        for (auto& elt: vec | std::views::reverse) {
            if (is_head_this_symbol(elt, "splice-unquote")) {
                result = make_list(
                        "concat",
                        mal_list_at(mal_to<mal_list>(elt), 1),
                        result);
            } else {
                result = make_list("cons", quasiquote(elt), result);
            }
        }
        if (list_ptr->is_vector()) {
            //TODO: Use become_list?
            return make_list("vec", result);
        }
        return result;
    }
    if (auto symbol_ptr{std::get_if<std::string>(&ast)}; symbol_ptr) {
        return make_list("quote", ast);
    }
    if (auto map_ptr{std::get_if<mal_map>(&ast)}; map_ptr) {
        return make_list("quote", ast);
    }
#if 1
    return ast; // need to quote?
#else
    return make_list("quote", ast);
#endif
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
                for (auto element: cpp_vector) {
                    mal_list_add(results, eval(element, current_env));
                }
                return results;
            }
            if (list->empty()) return ast;
            const mal_type head{mal_list_at(*list, 0)};
            if (auto symbol{std::get_if<std::string>(&head)}; symbol) {
                if ("def!" == *symbol) {
                    mal_type value{eval(mal_list_at(*list, 2), current_env)};
                    current_env->set(list->at_to<std::string>(1), value);
                    return value;
                }
                if ("let*" == *symbol) {
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
                if ("do" == *symbol) {
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
                if ("if" == *symbol) {
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
                if ("fn*" == *symbol) {
                    return mal_func{*list, current_env};
                }
                if ("quote" == *symbol) {
                    return mal_list_at(*list, 1);
                }
                if ("quasiquote" == *symbol) {
                    // TCO:
                    ast = quasiquote(mal_list_at(*list, 1));
                    continue;
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

int main(int argc, const char** argv)
{
    try {
        // Capture the command line args:
        std::vector<std::string_view> args(argv + 1, argv + argc);
        // Create the environment for the REPL:
        auto repl_env{env::make(get_ns())};
        // Check for flags:
        //(Not sure if this is a good idea...)
        for (auto arg: args) {
            if (arg.starts_with("--")) {
                auto equals{arg.find('=')};
                if (arg.npos == equals) {
                    auto symbol{arg.substr(2)};
                    repl_env->set(symbol, mal_true{});
                } else {
                    auto symbol{arg.substr(2, equals - 2)};
                    auto value{arg.substr(equals + 1)
                        | std::ranges::to<std::vector<char>>()};
                    repl_env->set(symbol, value);
                }
            }
        }
        std::erase_if(args, [](auto arg){ return arg.starts_with("--"); });
        // Add some stuff to the environment:
        rep("(def! not (fn* (a) (if a false true)))", repl_env);
        auto core_eval = [repl_env](const mal_list& args) -> mal_type
        {
            return eval(mal_list_at(args, 0), repl_env);
        };
        repl_env->set("eval", mal_proc{core_eval});
        rep("(def! load-file (fn* (f) (eval (read-string (str \"(do \" (slurp f) \"\nnil)\")))))", repl_env);
        // If we didn't get any command line args, start the REPL.
        if (args.empty()) {
            rep("(def! *ARGV* (list))", repl_env);
            while (true) {
                std::cout << "user> ";
                std::string line;
                if (not std::getline(std::cin, line)) break;
                try {
                    std::cout << rep(line, repl_env) << '\n';
                } catch (const comment_exception&) {
                    continue;
                } catch (const std::exception& e) {
                    std::cout << "Exception: " << e.what() << '\n';
                }
            }
        } else {
            // Add the command line arguments to *ARGV*.
            // Don't include the filename of the program to run.
            std::string argv_string{"(def! *ARGV* (list"};
            std::ranges::subrange argv_rest{args.begin() + 1, args.end()};
            for (auto arg: argv_rest) {
                argv_string += ' ';
                argv_string += encode_string(arg);
            }
            argv_string += "))";
            rep(argv_string, repl_env);
            // Run the file given on the command line.
            std::string load_string{"(load-file "};
            load_string += encode_string(args.at(0));
            load_string += ')';
            rep(load_string, repl_env);
        }
    } catch (const std::exception& e) {
        std::cout << "\nEXCEPTION: " << e.what() << '\n';
    }
}


#include "reader.hpp"
#include "types.hpp"
#include <exception>
#include <iostream>
#include <print>
#include <string>
#include <string_view>

Value read(std::string_view s)
{
    return read_str(s);
}

Value eval(const Value& ast, Env_ptr current_env_ptr)
{
    return ast;
}

std::string print(const Value& value)
{
    return to_string(value, true);
}

std::string rep(std::string_view s, Env_ptr repl_env)
{
    return print(eval(read(s), repl_env));
}

int main()
{
    while (true) {
        std::print("user> ");
        std::string line;
        if (not std::getline(std::cin, line)) break;
        try {
            std::println("{}", rep(line, nullptr));
        } catch (const std::exception& e) {
            std::println("Exception: {}", e.what());
        }
    }
}

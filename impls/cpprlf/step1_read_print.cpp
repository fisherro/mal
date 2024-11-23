#include "printer.hpp"
#include "reader.hpp"
#include <exception>
#include <iostream>
#include <string>
#include <string_view>

#include "fake_eval.hpp"

// Including <print> caused:
// g++: internal compiler error: File size limit exceeded signal terminated program cc1plus

auto read(std::string_view s)
{
    return read_str(s);
}

auto eval_(const auto& ast)
{
    return ast;
}

std::string print(const mal_type& mt)
{
    return pr_str(mt, true);
}

std::string rep(std::string_view s)
{
    return print(eval_(read(s)));
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


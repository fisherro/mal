#include <iostream>
#include <string>
#include <string_view>

#include "fake_eval.hpp"

std::string read(std::string_view s)
{
    return std::string{s};
}

std::string eval_(std::string_view s)
{
    return std::string{s};
}

std::string print(std::string_view s)
{
    return std::string{s};
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
        std::cout << rep(line) << '\n';
    }
}

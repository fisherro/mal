#include <iostream>
#include <string>
#include <string_view>

std::string read(std::string_view s)
{
    return std::string{s};
}

std::string eval(std::string_view s)
{
    return std::string{s};
}

std::string print(std::string_view s)
{
    return std::string{s};
}

std::string rep(std::string_view s)
{
    return print(eval(read(s)));
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

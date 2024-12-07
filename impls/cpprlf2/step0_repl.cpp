#include <iostream>
#include <print>
#include <string>

// Just a dummy so that we can get past the step 0 tests
int main()
{
    while (true) {
        std::print("user> ");
        std::string line;
        if (not std::getline(std::cin, line)) break;
        std::println("{}", line);
    }
}

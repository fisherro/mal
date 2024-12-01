#include "env.hpp"
#include "printer.hpp"
#include <memory>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>

std::shared_ptr<env> env::make(std::shared_ptr<env> outer)
{
    auto an_env{new env{outer}};
    return std::shared_ptr<env>{an_env};
}

bool env::has(std::string_view key) const
{
    //TODO: Loop instead of recursing?
    bool found{my_data.contains(std::string(key))};
    if (found) return true;
    if (my_outer) return my_outer->has(key);
    return false;
}

void env::set(std::string_view key, mal_type value)
{
    my_data[std::string(key)] = value;
}

mal_type env::get(std::string_view key) const
{
    //TODO: Loop instead of recursing?
    auto iter{my_data.find(std::string(key))};
    if (my_data.end() != iter) return iter->second;
    if (my_outer) return my_outer->get(key);
    //TODO: Make subclass of runtime_error?
    throw std::runtime_error{std::string(key) + " not found"};
}

void env::dump(bool full) const
{
    const std::string prefix{"EVAL DUMP: "};
    std::println("{}env {}", prefix, reinterpret_cast<const void*>(this));
    for (auto& [key, value]: my_data) {
        std::println("{}{}:{}:{}",
                prefix, key, get_mal_type(value), pr_str(value, true));
    }
    if (my_outer and full) {
        std::println("{}next environment...", prefix);
        my_outer->dump(true);
    }
}


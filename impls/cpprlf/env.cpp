#include "env.hpp"
#include <iostream>
#include <memory>
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

void env::dump(std::ostream& out) const
{
    const std::string prefix{"EVAL DUMP: "};
    out << prefix << "env " << this << '\n';
    for (auto& [key, value]: my_data) {
        out << prefix
            << key << ":"
            << get_mal_type(value) << ":"
            << pr_str(value, true) << '\n';
    }
    if (my_outer) {
        out << prefix << "next environment...\n";
        my_outer->dump(out);
    }
}


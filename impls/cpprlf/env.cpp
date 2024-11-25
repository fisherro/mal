#include "env.hpp"
#include <stdexcept>

std::shared_ptr<env> env::make(std::shared_ptr<env> outer)
{
    auto an_env{new env{outer}};
    return std::shared_ptr<env>{an_env};
}

bool env::has(const std::string& key) const
{
    //TODO: Loop instead of recursing?
    bool found{my_data.contains(key)};
    if (found) return true;
    if (my_outer) return my_outer->has(key);
    return false;
}

void env::set(const std::string& key, mal_type value)
{
    my_data[key] = value;
}

mal_type env::get(const std::string& key) const
{
    //TODO: Loop instead of recursing?
    auto iter{my_data.find(key)};
    if (my_data.end() != iter) return iter->second;
    if (my_outer) return my_outer->get(key);
    //TODO: Make subclass of runtime_error?
    throw std::runtime_error{key + " not found"};
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


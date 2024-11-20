#pragma once
#include "types.hpp"
#include <stdexcept>
#include <string>
#include <unordered_map>

//TODO The keys are actually symbols.
// Once we have a string/symbol distinction, we'll need to fix this.
struct env {
    explicit env(const env* outer = nullptr): my_outer{outer} {}
    bool has(const std::string& key) const { return my_data.contains(key); }
    void set(const std::string& key, mal_type value) { my_data[key] = value; }
    mal_type get(const std::string& key) const
    {
        //TODO: Loop instead of recursing?
        auto iter{my_data.find(key)};
        if (my_data.end() != iter) return iter->second;
        if (my_outer) return my_outer->get(key);
        throw std::runtime_error{key + " not found"};
    }
private:
    const env* my_outer{nullptr};
    std::unordered_map<std::string, mal_type> my_data;
};


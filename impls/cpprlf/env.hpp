#pragma once
#include "types.hpp"
#include "printer.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

//TODO Move member function definitions to env.cpp.
//TODO The keys are actually symbols.
// Once we have a string/symbol distinction, we'll need to fix this.
struct env {
    static std::shared_ptr<env> make(
            std::shared_ptr<env> outer = std::shared_ptr<env>{});

    bool has(const std::string& key) const;
    void set(const std::string& key, mal_type value);
    mal_type get(const std::string& key) const;
    void dump(std::ostream& out) const;
    
private:
    env() = delete;
    env(const env&) = delete;
    env& operator=(const env&) = delete;
    env(env&&) = delete;
    env& operator=(env&&) = delete;

    explicit env(std::shared_ptr<env> outer): my_outer{outer} {}

    std::shared_ptr<env> my_outer;
    std::unordered_map<std::string, mal_type> my_data;
};


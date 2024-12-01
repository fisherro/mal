#pragma once
#include "types.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

struct env {
    static std::shared_ptr<env> make(
            std::shared_ptr<env> outer = std::shared_ptr<env>{});

    bool has(std::string_view key) const;
    void set(std::string_view key, mal_type value);
    mal_type get(std::string_view key) const;
    void dump(bool full = false) const;
    
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


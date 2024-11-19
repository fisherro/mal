#pragma once
#include <any>
#include <string>
#include <variant>
#include <vector>

/*
 * A mal_list is a list of mal_types.
 * A mal_type is a variant that includes a mal_list.
 * To break the recursion, we use type erasure in the form of std::any.
 * So while a mal_list is a std::vector<mal_type>,
 * The actual type we use is std::vector<std::any>.
 */
using mal_list = std::vector<std::any>;
using mal_type = std::variant<int, std::string, mal_list>;


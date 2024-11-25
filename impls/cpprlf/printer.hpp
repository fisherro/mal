#pragma once
#include "types.hpp"
#include <string>

std::string pr_str(const mal_type& t, bool print_readably);
std::string encode_string(std::string_view s);


#pragma once
#include "types.hpp"
#include <string_view>

struct comment_exception {};

mal_type read_str(std::string_view s);


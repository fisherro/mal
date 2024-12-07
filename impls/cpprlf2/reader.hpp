#pragma once
#include "types.hpp"
#include <string_view>

struct Comment_exception {};

Value read_str(std::string_view s);


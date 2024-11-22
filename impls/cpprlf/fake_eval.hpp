#pragma once
// This is to allow steps before 5 to still compile.
#include "types.hpp"
mal_type eval(mal_type, std::shared_ptr<env>) { return mal_nil{}; }

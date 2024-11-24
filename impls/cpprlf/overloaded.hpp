#pragma once

template <typename... Args>
struct overloaded: Args... { using Args::operator()...; };


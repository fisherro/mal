#pragma once
#include "types.hpp"
#include <string>

std::string to_string(const Value& value, bool readably);

inline std::string to_string(int n, bool) { return std::to_string(n); }
std::string to_string(const String& string, bool readably);
inline std::string to_string(const Symbol& symbol, bool) { return symbol; }
inline std::string to_string(const Keyword& kw, bool) { return kw; }
inline std::string to_string(Nil, bool) { return "nil"; }
inline std::string to_string(False, bool) { return "false"; }
inline std::string to_string(True, bool) { return "true"; }
std::string to_string(const List& list, bool readably);
std::string to_string(const Vector& vector, bool readably);
std::string to_string(const Map& map, bool readably);
std::string to_string(Atom atom, bool readably);
inline std::string to_string(const Cpp_func&, bool) { return "#<C++function>"; }
inline std::string to_string(const Mal_func&, bool) { return "#<function>"; }

std::string to_string(const Key& key, bool readably);
std::string to_string(const Box& value, bool readably);

inline std::string pr_str(const Value& value, bool readably)
{ return to_string(value, readably); }


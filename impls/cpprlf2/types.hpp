#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

/*
 * Deriving from some Standard Library types (particularly the STL) is not
 * considered a "best practice". But it appears to be perfectly valid with
 * some caveats.
 */

//TODO: Add some constexpr and noexcept.

struct String   : std::string {};
struct Symbol   : std::string {};
struct Keyword  : std::string {};
struct Nil      : private std::monostate {};
struct False    : private std::monostate {};
struct True     : private std::monostate {};
struct Box;
using  Atom     = std::shared_ptr<Box>;
struct List     : std::vector<Box> {};
struct Vector   : std::vector<Box> {};
using  Key      = std::variant<String, Keyword>;
struct Key_hash { std::size_t operator()(const Key& key) const noexcept; };
struct Map      : std::unordered_map<Key, Box, struct Key_hash> {};
struct Cpp_func;
struct Mal_func;

/*
 * Value: A variant that can hold all the types used in mal code
 */
using Value = std::variant<
    int, String, Keyword, Symbol,
    Nil, False, True,
    List, Vector, Map, Atom,
    Cpp_func, Mal_func>;

struct Env {};
using Env_ptr = std::shared_ptr<Env>;

struct Cpp_func {
    std::function<Value(List)> _func;
};

/*
 * Mal_func: A function defined within mal code
 *
 * The _ast can't be a Value because Value depends on Box and box hasn't been
 * defined yet. If Box was defined before Mal_func, it would have the same
 * issue with Value depending on Mal_func and Mal_func not being defined yet.
 * Given the choice between Mal_func dynamically allocating _ast or Box
 * dynamically allocating its value, _ast is the one I choose to dynamically
 * allocate.
 */
struct Mal_func {
    std::string to_string(bool) const noexcept { return "#<function>"; }
    bool _is_macro{false};
    List _params;
    Atom _ast;
#if 0
    Env_ptr _env;
#endif
};

/*
 * Box: A wrapper around Value to break recursion
 *
 * A std::variant cannot directly contain itself, but it can contain a wrapper
 * type like Box.
 */
struct Box {
    Box() = default;
    template <typename T> Box(T x): _value{x} {}
    operator Value() { return _value; }
#if 0
    std::string to_string(bool print_readably) const
    { return ::to_string(_value, print_readably); }
#endif
    Value unbox() const { return _value; }
    Value _value;
};

inline char get_opener(const List&) { return '('; }
inline char get_opener(const Vector&) { return '['; }
inline char get_closer(const List&) { return ')'; }
inline char get_closer(const Vector&) { return ']'; }

Map boxen_to_map(const std::vector<Box>&);


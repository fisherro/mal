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
 *
 * Nil, False, & True all derive, privately, from std::monostate. There
 * doesn't appear to be any issues here.
 *
 * String, Symbol, & Keyword all derive from std::string. But we don't add
 * any data members. We only add member functions to give each specific
 * behavior.
 *
 * Likewise, List & Vector derive from std::vector<Box> to provide member
 * functions with different behavior. And Map derives from std::unordered_map.
 */

//TODO: Add some constexpr and noexcept.

struct Number;
struct String;
struct Symbol;
struct Keyword;
struct Nil  : private std::monostate {
    std::string to_string(bool) const noexcept { return "nil"; } };
struct False: private std::monostate {
    std::string to_string(bool) const noexcept { return "false"; } };
struct True : private std::monostate {
    std::string to_string(bool) const noexcept { return "true"; } };
struct Box;
struct Atom;
struct List;
struct Vector;
using  Key  = std::variant<String, Keyword>;
struct Map;
struct Cpp_func;
struct Mal_func;

/*
 * Value: A variant that can hold all the types used in mal code
 */
using Value = std::variant<
    Number, String, Keyword, Symbol,
    Nil, True, False,
    List, Vector, Map, Atom,
    Cpp_func, Mal_func>;

struct Env {};
using Env_ptr = std::shared_ptr<Env>;

std::string to_string(const Value& value, bool readably);

struct String: std::string {
    using std::string::string;
    std::string to_string(bool print_readably) const noexcept;
};

struct Number {
    std::string to_string(bool readably) const { return std::to_string(_n); }
    int _n;
};

/*
 * Symbol: The key used to store/lookup values in an Env
 *
 * Since String is its own type (an alias for std::string), Symbol being its
 * own type will make it easier to distinguish Strings and Symbols. Eventually
 * Symbols could be interned.
 */
struct Symbol: std::string {
    std::string to_string(bool) const noexcept { return *this; }
};

struct Keyword: std::string {
    std::string to_string(bool) const noexcept { return *this; }
};

inline std::string key_to_string(const Key& key, bool readably) noexcept
{
    return std::visit(
            [readably](const auto& k){ return k.to_string(readably); },
            key);
}

struct Key_hash {
    std::size_t operator()(const Key& key) const noexcept
    {
#if 0
        return std::visit(
                [](const auto& k){ return std::hash<std::string>{}(k.to_string(false)); },
                k);
#else
        return std::hash<std::string>{}(key_to_string(key, false));
#endif
    }
};

struct Atom {
    std::string to_string(bool readably) const;
    std::shared_ptr<Box> _ptr;
};

struct List: std::vector<Box> {
    char opener() const noexcept { return '('; }
    char closer() const noexcept { return ')'; }
    std::string to_string(bool readably) const;
};

struct Vector: std::vector<Box> {
    char opener() const noexcept { return '['; }
    char closer() const noexcept { return ']'; }
    std::string to_string(bool readably) const;
};

struct Map: std::unordered_map<Key, Box, Key_hash> {
    std::string to_string(bool readably) const;
};

/*
 * Cpp_func: A C++ function that is callable from mal code.
 */
struct Cpp_func {
    std::string to_string(bool) const noexcept { return "#<cpp_function>"; }
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
    std::string to_string(bool print_readably) const
    { return ::to_string(_value, print_readably); }
    Value unbox() const { return _value; }
    Value _value;
};

inline std::string Atom::to_string(bool readably) const
{
    if (not _ptr) return "(atom)";
    return std::string("(atom ")
        + _ptr->to_string(readably)
        + ")";
}

Map boxen_to_map(const std::vector<Box>&);


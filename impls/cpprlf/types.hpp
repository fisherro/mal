#pragma once
#include <any>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeindex>
#include <variant>
#include <vector>

/*
 * The mal_type variant is recursive. It holds types that depend on itself.
 * We use type erasure via std::any to break the recursion.
 *
 * While mal_list is a std::vector<std::any> it only holds mal_type elements.
 * While mal_proc return a std::any, that result always holds a mal_type.
 */

struct mal_nil { bool operator==(mal_nil) const {return true;} };
struct mal_false { bool operator==(mal_false) const {return true;} };
struct mal_true { bool operator==(mal_true) const {return true;} };

struct mal_list {
    friend struct mal_list_helper;
    mal_list() {}
    explicit mal_list(char opener): my_opener{opener} {}
    //TODO: Use operator<=>?
    bool operator==(const mal_list& that) const;

    bool empty() const { return my_elements.empty(); }
    std::size_t size() const { return my_elements.size(); }

    char get_opener() const { return my_opener; }
    char get_closer() const
    {
        if ('(' == my_opener) return ')';
        if ('[' == my_opener) return ']';
        if ('{' == my_opener) return '}'; //TODO: eventually goes away?
        return '\0';
    }

    bool is_list() const { return '(' == my_opener; }
    bool is_vector() const { return '[' == my_opener; }

    template <typename T>
    T at_to(std::size_t i) const;

private:
    // Used to distinguish "lists" from "vectors".
    char my_opener{'('};
    std::vector<std::any> my_elements;
};

template <typename T>
concept is_mal_list = std::is_same_v<mal_list, std::remove_const_t<T>>;

// This wraps a C++ function. Mal functions are mal_func.
struct mal_proc {
    friend struct mal_proc_helper;
    explicit mal_proc(std::function<std::any(mal_list)> f): my_function{f} {}
    bool operator==(const mal_proc&) const { return true; }
private:
    std::function<std::any(mal_list)> my_function;
};

// Forward declaration of env for use by mal_func:
struct env;

struct mal_func {
    friend struct mal_func_helper;
    explicit mal_func(mal_list ast, std::shared_ptr<env> current_env);
    bool operator==(const mal_func&) const { return true; }
    std::shared_ptr<env> make_env(const mal_list& args) const;
    
private:
    // This is the list of the function's parameter names.
    mal_list my_params;
    // This is the body of the function. It is actually a mal_type.
    std::any my_ast;
    // This is the "closed over" environment:
    std::shared_ptr<env> my_env;
    // This is a "precompiled" form of the function to be used with apply.
    mal_proc my_proc{[](mal_list)->std::any{return mal_nil{};}};
};

/*
 * To add a type to this, be sure...
 *
 *      - It has an op==.
 *      - It's been added to the visitor in printer.cpp
 *
 * ...else you'll get a nasty pile of compile errors.
 */
using mal_type = std::variant<
    int,
    std::string,       // used for symbols
    std::vector<char>, // used for strings
    mal_nil,
    mal_false,
    mal_true,
    mal_list,
    mal_proc,
    mal_func>;

struct mal_to_exception: std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct mal_at_exception: std::runtime_error {
    using std::runtime_error::runtime_error;
};

std::string demangle(const char* mangled);
std::string get_mal_type(const mal_type& m);
std::type_index get_mal_type_info(const mal_type& m);

template <typename T>
bool mal_is(const mal_type& m) { return std::holds_alternative<T>(m); }

inline bool mal_truthy(const mal_type& m)
{ return (not mal_is<mal_nil>(m)) and (not mal_is<mal_false>(m)); }

template <typename T>
T mal_to(const mal_type& m)
{
    const T* value_ptr{std::get_if<T>(&m)};
    if (not value_ptr) {
        std::ostringstream message;
        message << "requested type: "
            << demangle(typeid(T).name())
            << "; real type: "
            << get_mal_type(m);
        throw mal_to_exception{message.str()};
    }
    return *value_ptr;
}

template <typename T>
std::optional<T> try_mal_to(const mal_type& m)
{
    const T* value_ptr{std::get_if<T>(&m)};
    if (not value_ptr) return std::nullopt;
    return *value_ptr;
}

mal_type mal_list_at(const mal_list& list, std::size_t i);
std::optional<mal_type> try_mal_list_at(const mal_list& list, std::size_t i);
std::vector<mal_type> mal_list_get(const mal_list& list);
void mal_list_add(mal_list& list, mal_type m);

template <typename T>
T mal_list::at_to(std::size_t i) const
{ return mal_to<T>(mal_list_at(*this, i)); }

mal_type mal_proc_call(const mal_proc& p, const mal_list& args);
mal_type mal_func_ast(const mal_func& p);


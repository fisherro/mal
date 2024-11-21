#pragma once
#include <any>
#include <optional>
#include <stdexcept>
#include <sstream>
#include <string>
#include <typeindex>
#include <type_traits>
#include <variant>
#include <vector>
#include <functional>

/*
 * The mal_type variant is recursive. It holds types that depend on itself.
 * We use type erasure via std::any to break the recursion.
 *
 * While mal_list is a std::vector<std::any> it only holds mal_type elements.
 * While mal_proc return a std::any, that result always holds a mal_type.
 */

struct mal_list {
    friend struct mal_list_helper;
    //TODO: Use operator<=>?
    bool operator==(const mal_list& that) const;
private:
    std::vector<std::any> my_elements;
};

template <typename T>
concept is_mal_list = std::is_same_v<mal_list, std::remove_const_t<T>>;

struct mal_proc {
    explicit mal_proc(std::function<std::any(mal_list)> f): my_function{f} {}
    bool operator==(const mal_proc&) const { return true; }
    friend struct mal_proc_helper;
private:
    std::function<std::any(mal_list)> my_function;
};

struct mal_nil { bool operator==(mal_nil) const {return true;} };
struct mal_false { bool operator==(mal_false) const {return true;} };
struct mal_true { bool operator==(mal_true) const {return true;} };

using mal_type = std::variant<
    int,
    std::string,
    mal_nil,
    mal_false,
    mal_true,
    mal_list,
    mal_proc>;

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

bool mal_list_empty(const mal_list& list);
std::size_t mal_list_size(const mal_list& list);
mal_type mal_list_at(const mal_list& list, std::size_t i);
std::optional<mal_type> try_mal_list_at(const mal_list& list, std::size_t i);
std::vector<mal_type> mal_list_get(const mal_list& list);
void mal_list_add(mal_list& list, mal_type m);

template <typename T>
T mal_list_at_to(const mal_list& list, std::size_t i)
{ return mal_to<T>(mal_list_at(list, i)); }

mal_type mal_proc_call(const mal_proc& p, const mal_list& args);


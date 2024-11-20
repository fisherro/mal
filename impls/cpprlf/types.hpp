#pragma once
#include <any>
#include <optional>
#include <string>
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
private:
    std::vector<std::any> my_elements;
};

template <typename T>
concept is_mal_list = std::is_same_v<mal_list, std::remove_const_t<T>>;

struct mal_proc {
    explicit mal_proc(std::function<std::any(mal_list)> f): my_function{f} {}
    friend struct mal_proc_helper;
private:
    std::function<std::any(mal_list)> my_function;
};

struct mal_nil {};
struct mal_false {};
struct mal_true {};

using mal_type = std::variant<
    int,
    std::string,
    mal_nil,
    mal_false,
    mal_true,
    mal_list,
    mal_proc>;

template <typename T>
bool mal_is(const mal_type& m) { return std::holds_alternative<T>(m); }

inline bool mal_truthy(const mal_type& m)
{ return (not mal_is<mal_nil>(m)) and (not mal_is<mal_false>(m)); }

template <typename T>
T mal_to(const mal_type& m) { return std::get<T>(m); }

bool mal_list_empty(const mal_list& list);
mal_type mal_list_at(const mal_list& list, std::size_t i);
std::optional<mal_type> try_mal_list_at(const mal_list& list, std::size_t i);
std::vector<mal_type> mal_list_get(const mal_list& list);
void mal_list_add(mal_list& list, mal_type m);

template <typename T>
T mal_list_at_to(const mal_list& list, std::size_t i)
{ return mal_to<T>(mal_list_at(list, i)); }

mal_type mal_proc_call(const mal_proc& p, const mal_list& args);

std::string demangle(const char* mangled);
std::string get_mal_type(const mal_type& m);


#pragma once
#include <any>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <source_location>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeindex>
#include <variant>
#include <vector>

#ifdef MAL_FUNC_DEBUG
#include <print>
#endif

#ifdef MAL_FUNC_DEBUG
struct debug_me {
    void print(std::string_view mf, const debug_me* that = nullptr) const
    {
        std::print("debug_me({}): {} ({}",
                s, mf, reinterpret_cast<const void*>(this));
        if (that) {
            std::print(" <- {}", reinterpret_cast<const void*>(that));
        }
        std::println(")");
    }
    std::string s;
    debug_me() { print("ctor"); }
    debug_me(const std::string& s): s{s} { print("ctor(s)"); }
    debug_me(const debug_me& that): s{that.s} { print("copy ctor", &that); }
    debug_me(debug_me&& that): s{that.s} { print("move ctor", &that); }
    debug_me& operator=(const debug_me& that)
    { s = that.s; print("copy op=", &that); return *this; }
    debug_me& operator=(debug_me&& that)
    { s = that.s; print("move op=", &that); return *this; }
    ~debug_me() { print("dtor"); }
};
#endif

struct mal_meta_holder {
    void set_meta(std::shared_ptr<struct atom> metadata);
    std::shared_ptr<struct atom> get_meta() const { return my_metadata; }
private:
    std::shared_ptr<struct atom> my_metadata;
};

struct mal_nil { bool operator==(mal_nil) const {return true;} };
struct mal_false { bool operator==(mal_false) const {return true;} };
struct mal_true { bool operator==(mal_true) const {return true;} };

inline bool is_keyword(std::string_view symbol)
{ return (not symbol.empty()) and (':' == symbol.at(0)); }

/*
 * The mal_type variant is recursive. It holds types that depend on itself.
 * We use type erasure via std::any to break the recursion.
 *
 * While mal_list is a std::vector<std::any> it only holds mal_type elements.
 * While mal_proc return a std::any, that result always holds a mal_type.
 */

struct mal_list: mal_meta_holder {
    friend struct mal_list_helper;
    mal_list() {}
    explicit mal_list(char opener): my_opener{opener} {}
    //TODO: Use operator<=>?
    bool operator==(const mal_list& that) const;

    bool empty() const { return my_elements.empty(); }
    std::size_t size() const { return my_elements.size(); }

    mal_list rest() const;

    char get_opener() const { return my_opener; }
    char get_closer() const
    {
        if ('(' == my_opener) return ')';
        if ('[' == my_opener) return ']';
        if ('{' == my_opener) return '}';
        return '\0';
    }

    bool is_list()   const { return '(' == my_opener; }
    bool is_vector() const { return '[' == my_opener; }
    bool is_map()    const { return '{' == my_opener; }

    template <typename T>
    T at_to(std::size_t i) const;

    template <typename T>
    std::optional<T> try_at_to(std::size_t i) const;

    void become_list() { my_opener = '('; }
    void become_vector() { my_opener = '['; }

private:
    // Used to distinguish lists, vectors, & unevaluated maps.
    char my_opener{'('};
    std::vector<std::any> my_elements;
};

template <typename T>
concept is_mal_list = std::is_same_v<mal_list, std::remove_const_t<T>>;

struct mal_map: mal_meta_holder {
    friend struct mal_map_helper;
    bool operator==(const mal_map& that) const;

    // The keys returned are the internal key format.
    std::vector<std::string> inner_keys() const;

private:
    // The key is prefixed with 's' or 'k' for string or keyword.
    // (Mal strings are std::vector<char>;
    //  keywords are std::string that starts with colon.)
    // The value is a mal_type wrapped in a std::any.
    std::unordered_map<std::string, std::any> my_map;
};

// This wraps a C++ function. Mal functions are mal_func.
struct mal_proc: mal_meta_holder {
    friend struct mal_proc_helper;
    explicit mal_proc(std::function<std::any(mal_list)> f): my_function{f} {}
    bool operator==(const mal_proc&) const { return true; }
private:
    std::function<std::any(mal_list)> my_function;
};

// Forward declaration of env for use by mal_func:
struct env;

// The irony of the name "mal_func"...^_^
struct mal_func: mal_meta_holder {
    friend struct mal_func_helper;
    explicit mal_func(mal_list ast, std::shared_ptr<env> current_env);
    bool operator==(const mal_func&) const { return true; }
    std::shared_ptr<env> make_env(const mal_list& args) const;
    mal_proc proc() const;
    bool is_macro() const { return _is_macro; }
    void become_macro() { _is_macro = true; }

private:
    bool _is_macro{false};
    // This is the list of the function's parameter names.
    mal_list my_params;
    // This is the body of the function. It is actually a mal_type.
    std::any my_ast;
    // This is the "closed over" environment:
    std::shared_ptr<env> my_env;
#ifdef MAL_FUNC_DEBUG
    debug_me my_debug;
#endif
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
    mal_map,
    std::shared_ptr<struct atom>,
    mal_proc,
    mal_func>;

// Wouldn't "box" be a better name for this than "atom"?
struct atom {
    atom(const mal_type& m): my_value{m} {}
    mal_type deref() const { return my_value; }
    void reset(const mal_type& m) { my_value = m; }
private:
    mal_type my_value;
};

inline void mal_meta_holder::set_meta(std::shared_ptr<struct atom> metadata)
{ my_metadata = std::make_shared<atom>(*metadata); }

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
T mal_to(const mal_type& m,
        const std::source_location location = std::source_location::current())
{
    const T* value_ptr{std::get_if<T>(&m)};
    if (not value_ptr) {
        auto message{std::format(
                "{}({}): {}: requested type: {}; real type: {}",
                location.file_name(),
                location.line(),
                location.function_name(),
                demangle(typeid(T).name()),
                get_mal_type(m))};
        throw mal_to_exception{message};
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

//TODO: Look for places this could be used now that it is available.
mal_list mal_list_from(const std::span<mal_type> span, char opener = '(');
mal_type mal_list_at(const mal_list& list, std::size_t i);
std::optional<mal_type> try_mal_list_at(const mal_list& list, std::size_t i);
std::vector<mal_type> mal_list_get(const mal_list& list);
void mal_list_add(mal_list& list, mal_type m);

template <typename T>
T mal_list::at_to(std::size_t i) const
{ return mal_to<T>(mal_list_at(*this, i)); }

template <typename T>
std::optional<T> mal_list::try_at_to(std::size_t i) const
{
    auto opt{try_mal_list_at(*this, i)};
    if (not opt.has_value()) return std::nullopt;
    return try_mal_to<T>(*opt);
}

mal_map list_to_map(const mal_list& list);
std::string mal_map_okey_to_ikey(const mal_type& outer_key);
mal_type mal_map_ikey_to_okey(std::string_view inner_key);
void mal_map_set(mal_map& map, const mal_type& outer_key, mal_type value);
std::optional<mal_type> mal_map_get(
        const mal_map& map, const mal_type& outer_key);
void mal_map_remove(mal_map& map, const mal_type& outer_key);
std::vector<std::pair<std::string, mal_type>> mal_map_pairs(const mal_map& map);

mal_type mal_proc_call(const mal_proc& p, const mal_list& args);
mal_type mal_func_ast(const mal_func& p);


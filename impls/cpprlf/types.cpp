#include "types.hpp"
#include "env.hpp"
#include "printer.hpp"

#include <algorithm>
#include <any>
#include <print>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>
#include <variant>

#include <cxxabi.h>

struct mal_list_helper {
    static auto& get(is_mal_list auto& list)
    { return list.my_elements; }
};

struct mal_map_helper {
    static const auto& get(const mal_map& map)
    { return map.my_map; }
    static auto& get(mal_map& map)
    { return map.my_map; }
};

struct mal_proc_helper {
    static auto& get(const mal_proc& p) { return p.my_function; }
};

struct mal_func_helper {
    static mal_type ast(const mal_func& f)
    { return std::any_cast<mal_type>(f.my_ast); }
};

bool mal_list::operator==(const mal_list& that) const
{
#ifdef MAL_DEBUG_OP_EQUALS
    auto dump = [](std::string_view prefix, const auto& vec)
    {
        for (const auto& element: vec) {
            std::println("mal_list::op==({}): {}",
                    prefix, pr_str(element, true));
        }
    };
#endif
    // This can be harsh since we have to unwrap all the std::any.
    auto this_vec{mal_list_get(*this)};
    auto that_vec{mal_list_get(that)};
#ifdef MAL_DEBUG_OP_EQUALS
    dump("this", this_vec);
    dump("that", that_vec);
#endif
    return this_vec == that_vec;
}

mal_list mal_list_from(const std::span<mal_type> span, char opener)
{
    mal_list list{opener};
    for (auto& m: span) mal_list_add(list, m);
    return list;
}

mal_type mal_list_at(const mal_list& list, std::size_t i)
{
    std::optional<mal_type> element_opt{try_mal_list_at(list, i)};
    if (not element_opt) {
        std::ostringstream message;
        message << "Requesting index " << i
            << "; list size: " << list.size();
        throw mal_to_exception{message.str()};
    }
    return *element_opt;
}

std::optional<mal_type> try_mal_list_at(const mal_list& list, std::size_t i)
{
    try {
        return std::any_cast<mal_type>(mal_list_helper::get(list).at(i));
    } catch (const std::out_of_range&) {
        return std::nullopt;
    }
}

void mal_list_add(mal_list& list, mal_type m)
{
    mal_list_helper::get(list).push_back(m);
}

std::vector<mal_type> mal_list_get(const mal_list& list)
{
    auto& internal{mal_list_helper::get(list)};
    std::vector<mal_type> external;
    for (auto element: internal) {
        external.push_back(std::any_cast<mal_type>(element));
    }
    return external;
}

//TODO: Look for older code that could have used this.
mal_list mal_list::rest() const
{
    if (empty()) return *this;
    mal_list copy{*this};
    copy.my_elements.erase(copy.my_elements.begin());
    return copy;
}

mal_type mal_proc_call(const mal_proc& p, const mal_list& args)
{
    return std::any_cast<mal_type>(mal_proc_helper::get(p)(args));
}

std::string demangle(const char* mangled)
{
    int status{0};
    return abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
}

std::string get_mal_type(const mal_type& m)
{
    return demangle(get_mal_type_info(m).name());
}

// Weirdly, std::type_info can't be copied.
// But C++11 added a wrapper, std::type_index, which can.
std::type_index get_mal_type_info(const mal_type& m)
{
    auto get = [](auto&& arg)
    {
        return std::type_index(typeid(arg));
    };
    return std::visit(get, m);
}

mal_map list_to_map(const mal_list& list)
{
    mal_map result;
    if (0 == list.size()) return result;
    if (0 != (list.size() % 2)) throw std::runtime_error{"missing map value"};
    auto cpp_vector{mal_list_get(list)};
    //TODO: Look to see if span could replace uses of subrange.
    std::span<mal_type> span(cpp_vector);
    while (span.size() > 0) {
        mal_type outer_key{span[0]};
        mal_type value{span[1]};
        mal_map_set(result, outer_key, value);
        span = span.subspan(2);
    }
    return result;
}

/*
 * Keys (outer keys) can be either keywords or strings.
 *
 * Keywords are represented by a symbol (std::string) that begins with a colon.
 * Strings are represented by std::vector<char>.
 *
 * The inner key is a string prefixed by 's' for string keys and 'k' for
 * keyword keys.
 */
std::string mal_map_okey_to_ikey(const mal_type& outer_key)
{
    auto bad_key = [&outer_key]()
    {
        std::string type{get_mal_type(outer_key)};
        std::string value{pr_str(outer_key, true)};
        throw std::runtime_error{
            std::string{"bad map key type: "} + type + " " + value
        };
    };

    if (auto kw_ptr{std::get_if<std::string>(&outer_key)}; kw_ptr) {
        if (kw_ptr->empty() or (':' != kw_ptr->at(0))) bad_key();
        return std::string(1, 'k') + *kw_ptr;
    }
    if (auto s_ptr{std::get_if<std::vector<char>>(&outer_key)}; s_ptr) {
        std::string inner_key(1, 's');
        inner_key.append(s_ptr->begin(), s_ptr->end());
        return inner_key;
    }
    bad_key();
    // We'll never get here, but the compiler couldn't tell.
    return std::string{};
}

mal_type mal_map_ikey_to_okey(std::string_view inner_key)
{
    if (inner_key.empty()) {
        throw std::runtime_error{"invalid map inner key"};
    }
    if ('k' == inner_key[0]) {
        inner_key.remove_prefix(1);
        return mal_type{std::string{inner_key}};
    }
    if ('s' == inner_key[0]) {
        return std::vector<char>(inner_key.begin() + 1, inner_key.end());
    }
    throw std::runtime_error{"invalid map inner key"};
}

bool mal_map::operator==(const mal_map& that) const
{
    // This can be harsh since we have to unwrap all the std::any.
    auto& this_map{mal_map_helper::get(*this)};
    auto& that_map{mal_map_helper::get(that)};
    for (auto& this_pair: this_map) {
        auto that_iter{that_map.find(this_pair.first)};
        if (that_map.end() == that_iter) return false;
        const auto& this_value{std::any_cast<mal_type>(this_pair.second)};
        const auto& that_value{std::any_cast<mal_type>(that_iter->second)};
#ifdef MAL_DEBUG_OP_EQUALS
        std::println("mal_map::op==(this): {} -> {}",
                this_pair.first, pr_str(this_value, true));
        std::println("mal_map::op==(that): {} -> {}",
                that_iter->first, pr_str(that_value, true));
#endif
        if (this_value != that_value) return false;
    }
    return true;
}

std::vector<std::string> mal_map::inner_keys() const
{
    std::vector<std::string> some_keys;
    for (auto& pair: my_map) {
        some_keys.push_back(pair.first);
    }
    return some_keys;
}

std::vector<std::pair<std::string, mal_type>> mal_map_pairs(const mal_map& map)
{
    // May need to sort the output for op==?
    auto& inner_map{mal_map_helper::get(map)};
    std::vector<std::pair<std::string, mal_type>> pairs;
    for (auto [key, value]: inner_map) {
        pairs.push_back(std::make_pair(key, std::any_cast<mal_type>(value)));
    }
    return pairs;
}

void mal_map_set(mal_map& map, const mal_type& outer_key, mal_type value)
{
    std::string inner_key{mal_map_okey_to_ikey(outer_key)};
    auto& inner_map{mal_map_helper::get(map)};
    inner_map.insert_or_assign(inner_key, value);
}

std::optional<mal_type> mal_map_get(
        const mal_map& map, const mal_type& outer_key)
{
    std::string inner_key{mal_map_okey_to_ikey(outer_key)};
    auto& inner_map{mal_map_helper::get(map)};
    auto iter{inner_map.find(inner_key)};
    if (inner_map.end() == iter) return std::nullopt;
    return std::any_cast<mal_type>(iter->second);
}

void mal_map_remove(mal_map& map, const mal_type& outer_key)
{
    std::string inner_key{mal_map_okey_to_ikey(outer_key)};
    auto& inner_map{mal_map_helper::get(map)};
    inner_map.erase(inner_key);
}

mal_type mal_func_ast(const mal_func& f)
{
    return mal_func_helper::ast(f);
}

std::shared_ptr<env> mal_func::make_env(const mal_list& args) const
{
#ifdef MAL_FUNC_DEBUG
    std::println("{}: this = {}", __func__,
            reinterpret_cast<const void*>(&my_debug));
#endif

    auto mal_to_string = [](const mal_type& m) -> std::string
    {
        try { return mal_to<std::string>(m); }
        catch (const mal_to_exception& e) {
            throw mal_to_exception{
                std::string{e.what()}
                + "; A mal_func parameter was not a symbol!"};
        }
    };

    // Create a new environment:
    auto new_env{env::make(my_env)};
    // Convert my_params to a vector<string>:
    auto params_vector{
        mal_list_get(my_params)
        | std::views::transform(mal_to_string)
        | std::ranges::to<std::vector<std::string>>()
    };
    // Look for an ampersand; if found prepare for var arg handling.
    auto amp_iter{std::ranges::find(params_vector, "&")};
    std::string rest_param;
    if (params_vector.end() != amp_iter) {
        auto next{amp_iter + 1};
        if (params_vector.end() == next) {
            throw std::runtime_error{"no name for the rest parameter"};
        }
        rest_param = *next;
    }
    // Bind the regular parameters to the arguments in the new environment.
    std::ranges::subrange regular_params{params_vector.begin(), amp_iter};
    auto args_vector{mal_list_get(args)};
    auto arg_iter{args_vector.begin()};
    for (auto& param: regular_params) {
        if (arg_iter == args_vector.end()) break;
        new_env->set(param, *arg_iter);
        ++arg_iter;
    }
    // If we found an ampersand earlier, make a list of remaining args, and
    // bind the list to rest_param.
    if (params_vector.end() != amp_iter) {
        mal_list rest;
        std::ranges::subrange var_args{arg_iter, args_vector.end()};
        for (auto arg: var_args) {
            mal_list_add(rest, arg);
        }
        new_env->set(rest_param, rest);
    }
    return new_env;
}

mal_func::mal_func(mal_list ast, std::shared_ptr<env> current_env):
    my_params{ast.at_to<mal_list>(1)},
    my_ast{mal_list_at(ast, 2)},
    my_env{current_env}
#ifdef MAL_FUNC_DEBUG
    ,my_debug{pr_str(my_params, true) + pr_str(mal_list_at(ast, 2), true)}
#endif
{}

mal_proc mal_func::proc() const
{
    // We capture this by value. Otherwise, it gets destroyed before we use it.
    auto closure = [=, *this](const mal_list& args) -> mal_type
    {
        // Declaration of eval.
        // I don't currently have an appropriate header file to put this in.
        mal_type eval(mal_type, std::shared_ptr<env>);

        auto new_env{make_env(args)};
        return eval(mal_func_ast(*this), new_env);
    };
    return mal_proc{closure};
}


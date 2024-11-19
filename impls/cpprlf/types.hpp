#pragma once
#include <any>
#include <exception>
#include <functional>
#include <iomanip>
#include <sstream>
#include <source_location>
#include <string>
#include <variant>
#include <vector>

#include <cxxabi.h>

/*
 * The mal_type variant is recursive. It holds types that depend on itself.
 * We use type erasure via std::any to break the recursion.
 *
 * While mal_list is a std::vector<std::any> it only holds mal_type elements.
 * While mal_proc return a std::any, that result always holds a mal_type.
 */
using mal_list = std::vector<std::any>;
using mal_proc = std::function<std::any(mal_list)>;
using mal_type = std::variant<
    int,
    std::string,
    mal_list,
    mal_proc>;

inline std::string demangle(const char* mangled)
{
    int status{0};
    return abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
}

inline mal_type mal_cast(
        const std::any& any,
        const std::source_location location = std::source_location::current())
{
    try {
        return std::any_cast<mal_type>(any);
    } catch (const std::bad_any_cast& e) {
        std::ostringstream message;
        message << "std::bad_any_cast in "
            << std::quoted(location.function_name())
            << " in file " << std::quoted(location.file_name())
            << " at line " << location.line();
        if (not any.has_value()) {
            message << ": any doesn't hold a value";
        } else {
            message << ": type is " << demangle(any.type().name());
        }
        throw std::runtime_error(message.str());
    }
}

inline std::string get_mal_type(const mal_type& m)
{
    auto get = [](auto&& arg) -> std::string
    {
        return demangle(typeid(arg).name());
    };
    return std::visit(get, m);
}

inline std::string get_any_type(const std::any& a)
{
    return get_mal_type(mal_cast(a));
}


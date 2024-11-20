#include "types.hpp"

#include <any>
#include <string>
#include <vector>
#include <variant>

#include <cxxabi.h>

struct mal_list_helper {
    static auto& get(is_mal_list auto& list)
    { return list.my_elements; }
};

struct mal_proc_helper {
    static auto& get(const mal_proc& p) { return p.my_function; }
};

bool mal_list_empty(const mal_list& list)
{
    return mal_list_helper::get(list).empty();
}

mal_type mal_list_at(const mal_list& list, std::size_t i)
{ return std::any_cast<mal_type>(mal_list_helper::get(list).at(i)); }

void mal_list_add(mal_list& list, mal_type m)
{
    mal_list_helper::get(list).push_back(m);
}

std::vector<mal_type> mal_list_get(const mal_list & list)
{
    auto& internal{mal_list_helper::get(list)};
    std::vector<mal_type> external;
    for (auto& element: internal) {
        external.push_back(std::any_cast<mal_type>(element));
    }
    return external;
}

mal_type mal_proc_call(const mal_proc& p, const mal_list& args)
{
    return std::any_cast<mal_type>(mal_proc_helper::get(p)(args));
}

inline std::string demangle(const char* mangled)
{
    int status{0};
    return abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
}

std::string get_mal_type(const mal_type& m)
{
    auto get = [](auto&& arg) -> std::string
    {
        return demangle(typeid(arg).name());
    };
    return std::visit(get, m);
}


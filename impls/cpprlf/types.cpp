#include "types.hpp"

#include <any>
#include <stdexcept>
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

bool mal_list::operator==(const mal_list& that) const
{
    auto this_vec{mal_list_get(*this)};
    auto that_vec{mal_list_get(that)};
    return this_vec == that_vec;
}

bool mal_list_empty(const mal_list& list)
{
    return mal_list_helper::get(list).empty();
}

std::size_t mal_list_size(const mal_list& list)
{
    return mal_list_helper::get(list).size();
}

mal_type mal_list_at(const mal_list& list, std::size_t i)
{
    std::optional<mal_type> element_opt{try_mal_list_at(list, i)};
    if (not element_opt) {
        std::ostringstream message;
        message << "Requesting index " << i
            << "; list size: " << mal_list_size(list);
        throw mal_to_exception{message.str()};
    }
    return *element_opt;
}

std::optional<mal_type> try_mal_list_at(const mal_list& list, std::size_t i)
{
#if 0
    // For some reason, vector.size() is 1 when I used this version!
    auto vector{mal_list_helper::get(list)};
    if (i >= vector.size()) return std::nullopt;
    return std::any_cast<mal_type>(vector.at(i));
#else
    try {
        return std::any_cast<mal_type>(mal_list_helper::get(list).at(i));
    } catch (const std::out_of_range&) {
        return std::nullopt;
    }
#endif
}

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


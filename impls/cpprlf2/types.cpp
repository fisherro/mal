#include "printer.hpp"
#include "types.hpp"
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#if 0
#include <ranges> // Bring back if I can get original boxen_to_map working.
#endif

std::size_t Key_hash::operator()(const Key& key) const noexcept
{
    return std::hash<std::string>{}(to_string(key, false));
}

Map boxen_to_map(const std::vector<Box>& boxen)
{
    Map map;
    if (0 != (boxen.size() % 2)) {
        throw std::runtime_error{"unexpected end of input for map"};
    }
#if 0
    for (const auto& [key_box, value]: boxen | std::views::chunk(2)) {
        //TODO: Value to Key conversion should be done in Key itself?
        Value key_value{key_box.unbox()};
        Key key;
        if (Keyword* kp{std::get_if<Keyword>(&key_value)}; kp) {
            key = *kp;
        } else if (String* sp{std::get_if<String>(&key_value)}; sp) {
            key = *sp;
        } else {
            throw std::runtime_error{"invalid map key"};
        }
        map[key] = value;
    }
#else
    for (auto iter{boxen.begin()}; iter != boxen.end(); ++iter) {
        Value key_value{iter->unbox()};
        ++iter;
        Box value{*iter};
        //TODO: Value to Key conversion should be done in Key itself?
        Key key;
        if (Keyword* kp{std::get_if<Keyword>(&key_value)}; kp) {
            key = *kp;
        } else if (String* sp{std::get_if<String>(&key_value)}; sp) {
            key = *sp;
        } else {
            throw std::runtime_error{"invalid map key"};
        }
        map[key] = value;
    }
#endif
    return map;
}


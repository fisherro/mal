#include "types.hpp"
#include <ranges>
#include <string>
#include <unordered_map>
#include <variant>

std::string to_string(const Value& value, bool readably)
{
    return std::visit(
            [readably](auto x){ return x.to_string(readably); },
            value);
}

std::string to_string(const Key& key, bool readably)
{
    return std::visit(
            [readably](auto x){ return x.to_string(readably); },
            key);
}

std::string String::to_string(bool print_readably) const noexcept
{
    if (print_readably) {
        // double-quotes, newlines, and backslashes must be escaped
        // " -> \"
        // newline -> \n
        // backslash -> two backslashes
        //TODO...
        const std::unordered_map<char, std::string> map{
            { '"', "\\\"" },
            { '\n', "\\n" },
            { '\\', "\\\\" },
        };
        std::string s(1, '"');
        for (char c: *this) {
            auto iter{map.find(c)};
            if (map.end() == iter) s.push_back(c);
            else s += iter->second;
        }
        s += '"';
        return s;
    }
    return *this;
}

//TODO: Concept to limit this to List & Vector
std::string listish_to_string(const auto& listy, bool readably)
{
    std::string s(1, listy.opener());
    s += listy
        | std::views::transform(
                [readably](auto box){ return to_string(box, readably); })
        | std::views::join_with(std::string{" "})
        | std::ranges::to<std::string>();
    s += listy.closer();
    return s;
}

std::string List::to_string(bool readably) const
{ return listish_to_string(*this, readably); }

std::string Vector::to_string(bool readably) const
{ return listish_to_string(*this, readably); }

std::string Map::to_string(bool readably) const
{
    auto transformer = [readably](const auto& pair) -> std::string
    {
        auto& [key, value] = pair;
        return key_to_string(key, readably) + ' ' + value.to_string(readably);
    };

    std::string s(1, '{');
    s += *this
        | std::views::transform(transformer)
        | std::views::join_with(' ')
        | std::ranges::to<std::string>();
    s += '}';
    return s;
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


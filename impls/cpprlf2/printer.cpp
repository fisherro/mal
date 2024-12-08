#include "printer.hpp"
#include "types.hpp"
#include <format>
#include <ranges>
#include <string>
#include <unordered_map>
#include <variant>

std::string to_string(const Value& value, bool readably)
{
    return std::visit(
            [readably](auto x){ return to_string(x, readably); },
            value);
}

std::string to_string(const String& in_string, bool readably)
{
    if (readably) {
        // double-quotes, newlines, and backslashes must be escaped
        // " -> \"
        // newline -> \n
        // backslash -> two backslashes
        const std::unordered_map<char, std::string> map{
            { '"', "\\\"" },
            { '\n', "\\n" },
            { '\\', "\\\\" },
        };
        std::string out_string(1, '"');
        for (char c: in_string) {
            auto iter{map.find(c)};
            if (map.end() == iter) out_string.push_back(c);
            else out_string += iter->second;
        }
        out_string += '"';
        return out_string;
    }
    return in_string;
}

std::string boxen_to_string(const auto& boxen, bool readably)
{
    std::string s(1, get_opener(boxen));
    s += boxen
        | std::views::transform(
                [readably](auto box){ return to_string(box, readably); })
        | std::views::join_with(std::string{" "})
        | std::ranges::to<std::string>();
    s += get_closer(boxen);
    return s;
}

std::string to_string(const List& list, bool readably)
{ return boxen_to_string(list, readably); }

std::string to_string(const Vector& vector, bool readably)
{ return boxen_to_string(vector, readably); }

std::string to_string(const Map& map, bool readably)
{
    auto transformer = [readably](const auto& pair) -> std::string
    {
        auto& [key, value] = pair;
        return to_string(key, readably) + ' ' + to_string(value, readably);
    };

    std::string s(1, '{');
    s += map
        | std::views::transform(transformer)
        | std::views::join_with(' ')
        | std::ranges::to<std::string>();
    s += '}';
    return s;
}

std::string to_string(Atom atom, bool readably)
{
    if (not atom) return "(atom)";
    return std::format("(atom {})",
            to_string(atom->unbox(), readably));
}

std::string to_string(const Key& key, bool readably)
{
    return std::visit(
            [readably](const auto& x){ return to_string(x, readably); },
            key);
}

std::string to_string(const Box& box, bool readably)
{
    return to_string(box.unbox(), readably);
}


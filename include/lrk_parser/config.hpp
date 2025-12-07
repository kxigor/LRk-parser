#pragma once

#include <deque>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace lrk_parser {
using CharT = char;
using StringT = std::basic_string<CharT>;

using StringViewT = std::basic_string_view<CharT>;

template <typename T>
using VectorT = std::vector<T>;

template <typename T, typename H = std::hash<T>>
using UsetT = std::unordered_set<T, H>;

template <typename T, typename U, typename H = std::hash<T>>
using UmapT = std::unordered_map<T, U, H>;

template <typename T>
using DequeT = std::deque<T>;

template <typename T>
using OptionalT = std::optional<T>;

};  // namespace lrk_parser
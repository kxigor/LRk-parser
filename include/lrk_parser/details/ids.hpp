#pragma once

#include <cassert>
#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>

#include "lrk_parser/config.hpp"

namespace lrk_parser::details {

enum class SymbolId : std::size_t {};
enum class RuleId : std::size_t {};
enum class StateId : std::size_t {};

inline constexpr SymbolId kAugmentedStart{
    std::size_t{std::numeric_limits<std::make_unsigned_t<CharT>>::max()} + 1};
inline constexpr RuleId kStartRule{0};

constexpr SymbolId EncodeSymbol(CharT symbol) {
  return SymbolId{static_cast<std::make_unsigned_t<CharT>>(symbol)};
}

constexpr CharT DecodeSymbol(SymbolId symbol) {
  assert(symbol < kAugmentedStart);
  return static_cast<CharT>(std::to_underlying(symbol));
}

}  // namespace lrk_parser::details

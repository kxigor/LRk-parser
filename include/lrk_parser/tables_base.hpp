#pragma once

#include <cstddef>
#include <cstdint>

#include "config.hpp"
#include "details/ids.hpp"

namespace lrk_parser::details {

struct TransitionKey {
  [[nodiscard]] bool operator==(const TransitionKey& other) const = default;

  StateId current_state_id;
  SymbolId symbol;
};

struct TransitionKeyHash {
  [[nodiscard]] std::size_t operator()(
      const TransitionKey& tkey) const noexcept {
    // NOLINTBEGIN
    std::size_t seed = 0;
    seed ^= std::to_underlying(tkey.current_state_id) + 0x9e3779b9 +
            (seed << 6) + (seed >> 2);
    seed ^= static_cast<std::size_t>(tkey.symbol) + 0x9e3779b9 + (seed << 6) +
            (seed >> 2);
    // NOLINTEND
    return seed;
  }
};

enum class ActionType : std::uint8_t { Error, Shift, Reduce, Accept };

struct Action {
  [[nodiscard]] bool operator==(const Action& other) const = default;

  ActionType type = ActionType::Error;
  std::size_t value = 0;
};

struct ActionKey {
  [[nodiscard]] bool operator==(const ActionKey& other) const = default;

  StateId state_id;
  StringT lookahead;
};

struct ActionKeyHash {
  [[nodiscard]] std::size_t operator()(const ActionKey& key) const noexcept {
    // NOLINTBEGIN
    std::size_t seed = 0;
    seed ^= std::hash<StateId>{}(key.state_id) + 0x9e3779b9 + (seed << 6) +
            (seed >> 2);
    for (auto c : key.lookahead) {
      seed ^= std::hash<CharT>{}(c) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    // NOLINTEND
    return seed;
  }
};
}  // namespace lrk_parser::details
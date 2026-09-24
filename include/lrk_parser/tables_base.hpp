#pragma once

#include <cstddef>
#include <variant>

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

using TransitionMap = UmapT<TransitionKey, StateId, TransitionKeyHash>;

struct Shift {
  bool operator==(const Shift&) const = default;

  StateId next_state;
};

struct Reduce {
  bool operator==(const Reduce&) const = default;

  RuleId rule;
};

struct Accept {
  bool operator==(const Accept&) const = default;
};

struct Action {
  Action(Shift shift) : value(shift) {}
  Action(Reduce reduce) : value(reduce) {}
  Action(Accept accept) : value(accept) {}

  bool operator==(const Action&) const = default;

  std::variant<Shift, Reduce, Accept> value;
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

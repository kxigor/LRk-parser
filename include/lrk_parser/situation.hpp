#pragma once

#include "config.hpp"

namespace lrk_parser::details {
struct Situation {
  [[nodiscard]] bool operator==(const Situation& /*unused*/) const noexcept =
      default;

  std::size_t rule_idx{};
  std::size_t dot_pose{};
  StringT actpref;
};

struct SituationHash {
  /*=========== Hash ===========*/
  [[nodiscard]] std::size_t operator()(const Situation& sit) const noexcept {
    // NOLINTBEGIN
    std::size_t seed = 0;
    seed ^= sit.rule_idx + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= sit.dot_pose + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<StringT>{}(sit.actpref) + 0x9e3779b9 + (seed << 6) +
            (seed >> 2);
    // NOLINTEND
    return seed;
  }
};

using Situations = UsetT<Situation, SituationHash>;

struct SituationsHash {
  [[nodiscard]] std::size_t operator()(const Situations& set) const noexcept {
    // NOLINTBEGIN
    std::size_t seed = set.size();

    SituationHash situation_hasher;

    for (const auto& situation : set) {
      std::size_t situation_hash = situation_hasher(situation);
      seed ^= situation_hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    // NOLINTEND
    return seed;
  }
};
}  // namespace lrk_parser::details
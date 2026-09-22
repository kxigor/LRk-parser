#pragma once

#include <cstddef>

#include "config.hpp"
#include "details/ids.hpp"

namespace lrk_parser::details {
struct Situation {
  [[nodiscard]] bool operator==(const Situation& /*unused*/) const noexcept =
      default;

  RuleId rule_idx{};
  std::size_t dot_pose{};
  StringT actpref;
};

struct SituationHash {
  /*=========== Hash ===========*/
  [[nodiscard]] std::size_t operator()(const Situation& sit) const noexcept {
    // NOLINTBEGIN
    std::size_t seed = 0;
    seed ^= std::to_underlying(sit.rule_idx) + 0x9e3779b9 + (seed << 6) +
            (seed >> 2);
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
    std::size_t seed = set.size();
    for (const auto& situation : set) {
      seed += SituationHash{}(situation);
    }
    return seed;
  }
};
}  // namespace lrk_parser::details

#include "lrk_parser.hpp"

#include <cstddef>

std::size_t lrk_parser::LrkParser::SituationHash::operator()(
    const Situation& sit) const noexcept {
  // NOLINTBEGIN
  std::size_t seed = 0;
  seed ^= sit.rule_idx + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  seed ^= sit.dot_pose + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  seed ^= sit.lookahead_idx + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  // NOLINTEND
  return seed;
}

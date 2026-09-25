#pragma once

#include <cstddef>
#include <vector>

#include "ids.hpp"
#include "lrk_parser/config.hpp"

namespace lrk_parser::details {

struct Situation {
  bool operator==(const Situation&) const = default;

  RuleId rule{};
  std::size_t dot{};
  StringT lookahead;
};

using Situations = std::vector<Situation>;

}  // namespace lrk_parser::details

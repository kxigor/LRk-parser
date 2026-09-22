#pragma once

#include "config.hpp"

namespace lrk_parser {
struct Rule {
  CharT lhs{};
  StringT rhs;

  bool operator==(const Rule&) const = default;
};

}  // namespace lrk_parser

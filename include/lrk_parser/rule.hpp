#pragma once

#include <ostream>

#include "config.hpp"

namespace lrk_parser::details {
struct Rule {
  friend std::ostream& operator<<(std::ostream& os, const Rule& rule);

  CharT lhs{};
  StringT rhs;
};

std::ostream& operator<<(std::ostream& os, const Rule& rule);

}  // namespace lrk_parser::details
#pragma once

#include "config.hpp"

namespace lrk_parser {
struct Rule {
  CharT lhs{};
  StringT rhs;
};
}  // namespace lrk_parser
#pragma once

#include "config.hpp"

namespace lrk_parser::details {
struct Rule {
  CharT lhs{};
  StringT rhs;
};
}  // namespace lrk_parser::details
#include "lrk_parser/rule.hpp"

#include <ostream>

#include "lrk_parser/grammar.hpp"

std::ostream& lrk_parser::details::operator<<(
    std::ostream& os, const lrk_parser::details::Rule& rule) {
  os << rule.lhs << lrk_parser::Grammar::kArrow;
  if (rule.rhs.empty()) {
    os << "ε";
  } else {
    os << rule.rhs;
  }
  return os;
}
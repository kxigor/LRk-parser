#include <ostream>

#include "lrk_parser/output_helpers.hpp"

namespace lrk_parser {

std::ostream& operator<<(std::ostream& os, const Parser& parser) {
  return os << "Parser (k=" << parser.k_ << "):\n"
            << parser.grammar_ << parser.goto_table_ << parser.action_table_;
}

}  // namespace lrk_parser

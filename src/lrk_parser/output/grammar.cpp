#include <cstddef>
#include <ostream>
#include <utility>

#include "format.hpp"
#include "lrk_parser/output.hpp"

namespace lrk_parser {

std::ostream& operator<<(std::ostream& os, const Rule& rule) {
  output::PrintSymbol(os, rule.lhs);
  os << " -> ";
  output::PrintWord(os, rule.rhs);
  return os;
}

std::ostream& operator<<(std::ostream& os, const Grammar& grammar) {
  os << "Grammar\n  Nonterminals: ";
  output::PrintAlphabet(os, grammar.Nonterminals());
  os << "\n  Terminals: ";
  output::PrintAlphabet(os, grammar.Terminals());
  os << "\n  Start: ";
  output::PrintSymbol(os, grammar.Start());
  os << "\n  Rules:\n";
  for (std::size_t index = 0; index < grammar.Rules().size(); ++index) {
    os << "    #" << index << ' ' << grammar.Rules()[index] << '\n';
  }
  return os;
}

namespace details {

std::ostream& operator<<(std::ostream& os, const PreparedRule& rule) {
  output::PrintSymbol(os, rule.lhs);
  os << " -> ";
  if (rule.rhs.empty()) {
    os << "ε";
  }
  for (SymbolId symbol : rule.rhs) {
    output::PrintSymbol(os, symbol);
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const PreparedGrammar& grammar) {
  os << "Prepared rules:\n";
  for (std::size_t index = 0; index < grammar.Rules().size(); ++index) {
    os << "  #" << index << ' ' << grammar.Rules()[index] << '\n';
  }
  return os;
}

}  // namespace details
}  // namespace lrk_parser

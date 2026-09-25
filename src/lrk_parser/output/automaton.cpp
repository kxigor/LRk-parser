#include <algorithm>
#include <cstddef>
#include <ostream>
#include <utility>
#include <vector>

#include "format.hpp"
#include "lrk_parser/output.hpp"

namespace lrk_parser::details {

std::ostream& operator<<(std::ostream& os, const PrefixSet& set) {
  std::vector<StringT> words{set.begin(), set.end()};
  std::ranges::sort(words, output::WordLess);
  os << '{';
  bool first = true;
  for (const auto& word : words) {
    if (!first) {
      os << ", ";
    }
    output::PrintQuoted(os, word);
    first = false;
  }
  return os << '}';
}

std::ostream& operator<<(std::ostream& os, const FirstK& first) {
  std::vector<SymbolId> symbols;
  symbols.reserve(first.Sets().size());
  for (const auto& [symbol, prefixes] : first.Sets()) {
    symbols.push_back(symbol);
  }
  std::ranges::sort(symbols);
  os << "First_" << first.Lookahead() << ":\n";
  for (SymbolId symbol : symbols) {
    os << "  ";
    output::PrintSymbol(os, symbol);
    os << " = " << first.Sets().at(symbol) << '\n';
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Situation& situation) {
  os << "[Rule=" << std::to_underlying(situation.rule)
     << ", Dot=" << situation.dot << ", Lookahead=";
  output::PrintQuoted(os, situation.lookahead);
  return os << ']';
}

std::ostream& operator<<(std::ostream& os, const Situations& situations) {
  os << "{\n";
  for (const auto& situation : situations) {
    os << "    " << situation << '\n';
  }
  return os << '}';
}

std::ostream& operator<<(std::ostream& os, const TransitionKey& key) {
  os << '(' << std::to_underlying(key.current_state_id) << ", ";
  output::PrintSymbol(os, key.symbol);
  return os << ')';
}

std::ostream& operator<<(std::ostream& os,
                         const CanonicalCollection& collection) {
  os << "Canonical collection:\n";
  const auto& states = collection.States();
  for (std::size_t index = 0; index < states.size(); ++index) {
    os << "  State " << index << ":\n" << states[index] << '\n';
  }
  os << "Transitions:\n";
  return os << collection.Transitions();
}

}  // namespace lrk_parser::details

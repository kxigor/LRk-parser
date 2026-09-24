#include "lrk_parser/grammar.hpp"

#include <unordered_set>

namespace lrk_parser {

std::expected<Grammar, GrammarError> MakeGrammar(GrammarSpec spec) {
  for (CharT symbol : spec.terminals) {
    if (spec.nonterminals.contains(symbol)) {
      return std::unexpected{GrammarError{
          GrammarErrorKind::OverlappingAlphabets, symbol, std::nullopt}};
    }
  }

  if (!spec.nonterminals.contains(spec.start)) {
    return std::unexpected{
        GrammarError{GrammarErrorKind::InvalidStart, spec.start, std::nullopt}};
  }

  std::unordered_set<CharT> defined_nonterminals;
  for (std::size_t index = 0; index < spec.rules.size(); ++index) {
    const auto& rule = spec.rules[index];
    if (!spec.nonterminals.contains(rule.lhs)) {
      return std::unexpected{
          GrammarError{GrammarErrorKind::InvalidLhs, rule.lhs, index}};
    }
    for (CharT symbol : rule.rhs) {
      if (!spec.terminals.contains(symbol) &&
          !spec.nonterminals.contains(symbol)) {
        return std::unexpected{
            GrammarError{GrammarErrorKind::UnknownRhsSymbol, symbol, index}};
      }
    }
    defined_nonterminals.insert(rule.lhs);
  }

  if (!defined_nonterminals.contains(spec.start)) {
    return std::unexpected{GrammarError{GrammarErrorKind::MissingProduction,
                                        spec.start, std::nullopt}};
  }

  for (std::size_t index = 0; index < spec.rules.size(); ++index) {
    for (CharT symbol : spec.rules[index].rhs) {
      if (spec.nonterminals.contains(symbol) &&
          !defined_nonterminals.contains(symbol)) {
        return std::unexpected{
            GrammarError{GrammarErrorKind::MissingProduction, symbol, index}};
      }
    }
  }

  return Grammar{std::move(spec)};
}

}  // namespace lrk_parser

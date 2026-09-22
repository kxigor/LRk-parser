#include "lrk_parser/details/prepared_grammar.hpp"

namespace lrk_parser::details {

std::vector<SymbolId> EncodeSymbols(StringViewT symbols) {
  std::vector<SymbolId> result;
  result.reserve(symbols.size());
  for (CharT symbol : symbols) {
    result.push_back(EncodeSymbol(symbol));
  }
  return result;
}

PreparedGrammar::PreparedGrammar(const Grammar& grammar) {
  for (CharT symbol : grammar.Terminals()) {
    const auto id = EncodeSymbol(symbol);
    if (terminal_set_.insert(id).second) terminals_.push_back(id);
  }
  for (CharT symbol : grammar.Nonterminals()) {
    const auto id = EncodeSymbol(symbol);
    if (nonterminal_set_.insert(id).second) nonterminals_.push_back(id);
  }
  nonterminals_.push_back(kAugmentedStart);
  nonterminal_set_.insert(kAugmentedStart);

  rules_.push_back({kAugmentedStart, {EncodeSymbol(grammar.Start())}});
  rules_by_lhs_[kAugmentedStart].push_back(kStartRule);
  for (const auto& rule : grammar.Rules()) {
    const auto lhs = EncodeSymbol(rule.lhs);
    rules_by_lhs_[lhs].push_back(RuleId{rules_.size()});
    rules_.push_back({lhs, EncodeSymbols(rule.rhs)});
  }
}

std::span<const RuleId> PreparedGrammar::RulesFor(SymbolId symbol) const {
  const auto it = rules_by_lhs_.find(symbol);
  if (it == rules_by_lhs_.end()) return {};
  return it->second;
}

}  // namespace lrk_parser::details

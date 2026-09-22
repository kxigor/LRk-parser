#pragma once

#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ids.hpp"
#include "lrk_parser/config.hpp"
#include "lrk_parser/grammar.hpp"

namespace lrk_parser::details {

struct PreparedRule {
  SymbolId lhs;
  std::vector<SymbolId> rhs;
};

class PreparedGrammar {
 public:
  explicit PreparedGrammar(const Grammar& grammar);

  std::span<const SymbolId> Terminals() const { return terminals_; }
  std::span<const SymbolId> Nonterminals() const { return nonterminals_; }
  std::span<const PreparedRule> Rules() const { return rules_; }
  const PreparedRule& GetRule(RuleId rule) const {
    return rules_[std::to_underlying(rule)];
  }
  std::span<const RuleId> RulesFor(SymbolId symbol) const;
  bool IsTerminal(SymbolId symbol) const {
    return terminal_set_.contains(symbol);
  }
  bool IsNonterminal(SymbolId symbol) const {
    return nonterminal_set_.contains(symbol);
  }

 private:
  std::vector<SymbolId> terminals_;
  std::vector<SymbolId> nonterminals_;
  std::unordered_set<SymbolId> terminal_set_;
  std::unordered_set<SymbolId> nonterminal_set_;
  std::vector<PreparedRule> rules_;
  std::unordered_map<SymbolId, std::vector<RuleId>> rules_by_lhs_;
};

std::vector<SymbolId> EncodeSymbols(StringViewT symbols);

}  // namespace lrk_parser::details

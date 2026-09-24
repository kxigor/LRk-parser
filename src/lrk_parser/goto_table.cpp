#include "lrk_parser/goto_table.hpp"

#include <utility>

#include "lrk_parser/tables_base.hpp"

lrk_parser::details::GotoTable lrk_parser::details::GotoTable::Build(
    const PreparedGrammar& grammar, TransitionMap transitions) {
  for (auto it = transitions.begin(); it != transitions.end();) {
    if (!grammar.IsNonterminal(it->first.symbol)) {
      it = transitions.erase(it);
    } else {
      ++it;
    }
  }
  return GotoTable(std::move(transitions));
}

lrk_parser::details::GotoTable::GotoTable(TransitionMap transitions)
    : goto_table_(std::move(transitions)) {}

bool lrk_parser::details::GotoTable::HasGotoState(
    const TransitionKey& t_key) const {
  return goto_table_.contains(t_key);
}

const lrk_parser::details::StateId&
lrk_parser::details::GotoTable::GetGotoState(const TransitionKey& t_key) const {
  return goto_table_.at(t_key);
}

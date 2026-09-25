#include "lrk_parser/details/goto_table.hpp"

#include <utility>

#include "lrk_parser/details/tables_base.hpp"

lrk_parser::details::GotoTable lrk_parser::details::GotoTable::Build(
    const PreparedGrammar& grammar, TransitionMap transitions) {
  for (auto it = transitions.begin(); it != transitions.end();) {
    if (!grammar.IsNonterminal(it->first.symbol)) {
      it = transitions.erase(it);
    } else {
      ++it;
    }
  }
  return GotoTable{std::move(transitions)};
}

lrk_parser::details::GotoTable::GotoTable(TransitionMap transitions)
    : goto_table_{std::move(transitions)} {}

const lrk_parser::details::StateId* lrk_parser::details::GotoTable::FindState(
    const TransitionKey& key) const {
  const auto it = goto_table_.find(key);
  return it == goto_table_.end() ? nullptr : &it->second;
}

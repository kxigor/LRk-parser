#include "lrk_parser/goto_table.hpp"

bool lrk_parser::details::GotoTable::has_goto_state(
    const TransitionKey& t_key) const {
  return goto_table_.contains(t_key);
}

const lrk_parser::details::StateIdT&
lrk_parser::details::GotoTable::get_goto_state(
    const TransitionKey& t_key) const {
  return goto_table_.at(t_key);
}

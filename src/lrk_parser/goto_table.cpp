#include "lrk_parser/goto_table.hpp"

lrk_parser::details::GotoTable::GotoTable(
    const CanonicalCollection& lr_colletion)
    : goto_table_(lr_colletion.get_goto_table()) {}

lrk_parser::details::GotoTable::GotoTable(CanonicalCollection&& lr_colletion)
    : goto_table_(std::move(lr_colletion).take_goto_table()) {}

bool lrk_parser::details::GotoTable::has_goto_state(
    const TransitionKey& t_key) const {
  return goto_table_.contains(t_key);
}

const lrk_parser::details::StateIdT&
lrk_parser::details::GotoTable::get_goto_state(
    const TransitionKey& t_key) const {
  return goto_table_.at(t_key);
}

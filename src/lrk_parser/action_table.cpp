#include "lrk_parser/action_table.hpp"

#include <cstddef>
#include <format>
#include <stdexcept>

#include "lrk_parser/canonical_collection.hpp"
#include "lrk_parser/config.hpp"
#include "lrk_parser/first_k.hpp"
#include "lrk_parser/grammar.hpp"
#include "lrk_parser/tables_base.hpp"

using ActionTable = lrk_parser::details::ActionTable;

ActionTable::ActionTable(const Grammar& grammar, const FirstK& first_k,
                         const CanonicalCollection& lr_collection) {
  ATC ctx(grammar, first_k, lr_collection);
  build_action_table(ctx);
}

bool ActionTable::has_parse_action(const ActionKey& a_key) const {
  return action_table_.contains(a_key);
}

const lrk_parser::details::Action& ActionTable::get_parse_action(
    const ActionKey& a_key) const {
  return action_table_.at(a_key);
}

void ActionTable::build_action_table(ATC& ctx) {
  const auto& states = ctx.lr_collection.get_states();

  for (std::size_t state_idx = 0; state_idx < states.size(); ++state_idx) {
    process_state_situations(ctx, state_idx);
  }
}

void ActionTable::process_state_situations(ATC& ctx, std::size_t state_idx) {
  const auto& states = ctx.lr_collection.get_states();
  const auto& grammar = ctx.grammar;

  for (const auto& sit : states[state_idx]) {
    const auto& rule = grammar.get_rule_by_idx(sit.rule_idx);

    if (sit.dot_pose < rule.rhs.size()) {
      handle_shift_insert(ctx, state_idx, sit, rule);

    } else {
      if (rule.lhs == Grammar::kStarSym) {
        handle_accept_insert(state_idx, sit);
      } else {
        handle_reduce_insert(state_idx, sit);
      }
    }
  }
}

void ActionTable::handle_shift_insert(ATC& ctx, std::size_t state_idx,
                                      const Situation& sit, const Rule& rule) {
  const auto& grammar = ctx.grammar;
  const auto& goto_table = ctx.lr_collection.get_goto_table();
  const auto& first_k = ctx.first_k;

  const CharT kNextSym = rule.rhs[sit.dot_pose];

  if (grammar.is_terminal(kNextSym)) {
    const StringT kTail = rule.rhs.substr(sit.dot_pose);
    auto eff_lookaheads = first_k.compute_first_k(kTail + sit.actpref);

    for (const auto& u : eff_lookaheads) {
      const TransitionKey kTKey{.current_state_id = state_idx,
                                .symbol = kNextSym};
      if (goto_table.contains(kTKey)) {
        const StateIdT kNextState = goto_table.at(kTKey);
        add_action_checked(
            state_idx, u,
            Action{.type = ActionType::Shift, .value = kNextState});
      }
    }
  }
}

void ActionTable::handle_accept_insert(std::size_t state_idx,
                                       const Situation& sit) {
  if (sit.actpref.empty()) {
    add_action_checked(state_idx, sit.actpref,
                       Action{.type = ActionType::Accept, .value = 0});
  }
}

void ActionTable::handle_reduce_insert(std::size_t state_idx,
                                       const Situation& sit) {
  add_action_checked(state_idx, sit.actpref,
                     Action{.type = ActionType::Reduce, .value = sit.rule_idx});
}

void lrk_parser::details::ActionTable::add_action_checked(
    StateIdT state, const StringT& lookahead, Action new_action) {
  /*TODO: remove ugly code*/
  const ActionKey kAKey{.state_id = state, .lookahead = lookahead};
  if (action_table_.contains(kAKey)) {
    const auto& existing = action_table_.at(kAKey);
    if (existing == new_action) {
      return;
    }

    throw std::runtime_error(std::format(
        "LR(k) Conflict at state {}, lookahead '{}': existing type {}, new "
        "type {}",
        state, lookahead, static_cast<int>(existing.type),
        static_cast<int>(new_action.type)));
  }
  action_table_[kAKey] = new_action;
}

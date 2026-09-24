#include "lrk_parser/action_table.hpp"

#include <cstddef>
#include <format>
#include <stdexcept>

#include "lrk_parser/canonical_collection.hpp"
#include "lrk_parser/config.hpp"
#include "lrk_parser/details/prepared_grammar.hpp"
#include "lrk_parser/first_k.hpp"
#include "lrk_parser/tables_base.hpp"

using ActionTable = lrk_parser::details::ActionTable;

ActionTable::ActionTable(const PreparedGrammar& grammar, const FirstK& first_k,
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
  const auto& states = ctx.lr_collection.States();

  for (std::size_t state_idx = 0; state_idx < states.size(); ++state_idx) {
    process_state_situations(ctx, StateId{state_idx});
  }
}

void ActionTable::process_state_situations(ATC& ctx, StateId state_idx) {
  const auto& states = ctx.lr_collection.States();
  const auto& grammar = ctx.grammar;

  for (const auto& sit : states[std::to_underlying(state_idx)]) {
    const auto& rule = grammar.GetRule(sit.rule);

    if (sit.dot < rule.rhs.size()) {
      handle_shift_insert(ctx, state_idx, sit, rule);

    } else {
      if (sit.rule == kStartRule) {
        handle_accept_insert(state_idx, sit);
      } else {
        handle_reduce_insert(state_idx, sit);
      }
    }
  }
}

void ActionTable::handle_shift_insert(ATC& ctx, StateId state_idx,
                                      const Situation& sit,
                                      const PreparedRule& rule) {
  const auto& grammar = ctx.grammar;
  const auto& goto_table = ctx.lr_collection.Transitions();
  const auto& first_k = ctx.first_k;

  const SymbolId kNextSym = rule.rhs[sit.dot];

  if (grammar.IsTerminal(kNextSym)) {
    const auto kTail = std::span{rule.rhs}.subspan(sit.dot);
    auto eff_lookaheads = first_k.ForSequence(kTail, sit.lookahead);

    for (const auto& u : eff_lookaheads) {
      const TransitionKey kTKey{.current_state_id = state_idx,
                                .symbol = kNextSym};
      if (goto_table.contains(kTKey)) {
        const StateId kNextState = goto_table.at(kTKey);
        add_action_checked(state_idx, u,
                           Action{.type = ActionType::Shift,
                                  .value = std::to_underlying(kNextState)});
      }
    }
  }
}

void ActionTable::handle_accept_insert(StateId state_idx,
                                       const Situation& sit) {
  if (sit.lookahead.empty()) {
    add_action_checked(state_idx, sit.lookahead,
                       Action{.type = ActionType::Accept, .value = 0});
  }
}

void ActionTable::handle_reduce_insert(StateId state_idx,
                                       const Situation& sit) {
  add_action_checked(state_idx, sit.lookahead,
                     Action{.type = ActionType::Reduce,
                            .value = std::to_underlying(sit.rule)});
}

void lrk_parser::details::ActionTable::add_action_checked(
    StateId state, const StringT& lookahead, Action new_action) {
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
        std::to_underlying(state), lookahead, static_cast<int>(existing.type),
        static_cast<int>(new_action.type)));
  }
  action_table_[kAKey] = new_action;
}

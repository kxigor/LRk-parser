#include "lrk_parser/action_table.hpp"

lrk_parser::details::ActionTable::ActionTable(
    const Grammar& grammar, const FirstK& first_k,
    const CanonicalCollection& lr_collection) {
  build_action_table(grammar, first_k, lr_collection);
}

bool lrk_parser::details::ActionTable::has_parse_action(
    const ActionKey& a_key) const {
  return action_table_.contains(a_key);
}

const lrk_parser::details::Action&
lrk_parser::details::ActionTable::get_parse_action(
    const ActionKey& a_key) const {
  return action_table_.at(a_key);
}

void lrk_parser::details::ActionTable::build_action_table(
    const Grammar& grammar, const FirstK& first_k,
    const CanonicalCollection& lr_collection) {
  const auto& states = lr_collection.get_states();
  const auto& goto_table = lr_collection.get_goto_table();

  for (std::size_t i = 0; i < states.size(); ++i) {
    const StateIdT kCurrentStateId = i;
    const auto& situations = states[i];

    for (const auto& sit : situations) {
      const auto& rule = grammar.get_rule_by_idx(sit.rule_idx);

      if (sit.dot_pose < rule.rhs.size()) {
        const CharT kNextSym = rule.rhs[sit.dot_pose];

        if (grammar.is_terminal(kNextSym)) {
          const StringT kTail = rule.rhs.substr(sit.dot_pose);
          auto eff_lookaheads = first_k.compute_first_k(kTail + sit.actpref);

          for (const auto& u : eff_lookaheads) {
            const TransitionKey kTKey{.current_state_id = kCurrentStateId,
                                      .symbol = kNextSym};
            if (goto_table.contains(kTKey)) {
              const StateIdT kNextState = goto_table.at(kTKey);
              add_action_checked(
                  kCurrentStateId, u,
                  Action{.type = ActionType::Shift, .value = kNextState});
            }
          }
        }
      } else {
        if (rule.lhs == Grammar::kStarSym) {
          if (sit.actpref.empty()) {
            add_action_checked(kCurrentStateId, sit.actpref,
                               Action{.type = ActionType::Accept, .value = 0});
          }
        } else {
          add_action_checked(
              kCurrentStateId, sit.actpref,
              Action{.type = ActionType::Reduce, .value = sit.rule_idx});
        }
      }
    }
  }
}

void lrk_parser::details::ActionTable::add_action_checked(
    StateIdT state, const StringT& lookahead, Action new_action) {
  const ActionKey kAKey{.state_id = state, .lookahead = lookahead};
  if (action_table_.contains(kAKey)) {
    const auto& existing = action_table_.at(kAKey);
    if (existing == new_action) {
      return;
    }

    throw std::runtime_error(std::format(
        "LR(k) Conflict at state {}, lookahead '{}': existing type {}, new "
        "type {}",
        state, lookahead, (int)existing.type, (int)new_action.type));
  }
  action_table_[kAKey] = new_action;
}

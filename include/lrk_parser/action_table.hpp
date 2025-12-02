#pragma once

#include "tables_base.hpp"

namespace lrk_parser::details {
class ActionTable {
  using BaseActionTableT = UmapT<ActionKey, Action, ActionKeyHash>;

  void build_action_table() {
    for (std::size_t i = 0; i < states_.size(); ++i) {
      const StateIdT current_state_id = i;
      const auto& situations = states_[i];

      for (const auto& sit : situations) {
        const auto& rule = grammar_.rules[sit.rule_idx];

        if (sit.dot_pose < rule.rhs.size()) {
          const CharT next_sym = rule.rhs[sit.dot_pose];

          if (grammar_.is_terminal(next_sym)) {
            StringT tail = rule.rhs.substr(sit.dot_pose + 1);
            auto eff_lookaheads = compute_first_k_of_str(tail + sit.actpref);

            for (const auto& u : eff_lookaheads) {
              if (u.empty() || u[0] != next_sym) {
                continue;
              }

              TransitionKey tkey{current_state_id, next_sym};
              if (goto_table_.contains(tkey)) {
                StateIdT next_state = goto_table_.at(tkey);
                add_action_checked(current_state_id, u,
                                   Action{ActionType::Shift, next_state});
              }
            }
          }
        } else {
          if (rule.lhs == Grammar::kStarSym) {
            if (sit.actpref.empty()) {
              add_action_checked(current_state_id, sit.actpref,
                                 Action{ActionType::Accept, 0});
            }
          } else {
            add_action_checked(current_state_id, sit.actpref,
                               Action{ActionType::Reduce, sit.rule_idx});
          }
        }
      }
    }
  }

  void add_action_checked(StateIdT state, const StringT& lookahead,
                          Action new_action) {
    ActionKey key{state, lookahead};
    if (action_table_.contains(key)) {
      const auto& existing = action_table_.at(key);
      if (existing == new_action) return;

      throw std::runtime_error(std::format(
          "LR(k) Conflict at state {}, lookahead '{}': existing type {}, new "
          "type {}",
          state, lookahead, (int)existing.type, (int)new_action.type));
    }
    action_table_[key] = new_action;
  }

  BaseActionTableT action_table_;
};
}  // namespace lrk_parser::details
#pragma once

#include <cstddef>
#include <format>
#include <stdexcept>

#include "canonical_collection.hpp"
#include "config.hpp"
#include "first_k.hpp"
#include "grammar.hpp"
#include "tables_base.hpp"

namespace lrk_parser::details {
class ActionTable {
  /*====================== Usings/Helpers ======================*/
  using BaseActionTableT = UmapT<ActionKey, Action, ActionKeyHash>;

  /*================= Constructors/Destructors =================*/
 public:
  ActionTable() = default;

  ActionTable(const Grammar& grammar, const FirstK& first_k,
              const CanonicalCollection& lr_collection) {
    build_action_table(grammar, first_k, lr_collection);
  }

  ActionTable(const ActionTable& /*unused*/) = default;

  ActionTable(ActionTable&& /*unused*/) = default;

  ~ActionTable() = default;

  /*======================= Assignments ========================*/
  ActionTable& operator=(const ActionTable& /*unused*/) = default;

  ActionTable& operator=(ActionTable&& /*unused*/) = default;

  /*===================== Table Operations =====================*/
  [[nodiscard]] bool has_parse_action(const ActionKey& a_key) const {
    return action_table_.contains(a_key);
  }

  [[nodiscard]] const details::Action& get_parse_action(
      const ActionKey& a_key) const {
    return action_table_.at(a_key);
  }

  /*========================== Impls ===========================*/
 private:
  void build_action_table(const Grammar& grammar, const FirstK& first_k,
                          const CanonicalCollection& lr_collection) {
    const auto& states = lr_collection.get_states();
    const auto& goto_table = lr_collection.get_goto_table();

    for (std::size_t i = 0; i < states.size(); ++i) {
      const StateIdT kCurrentStateId = i;
      const auto& situations = states[i];

      for (const auto& sit : situations) {
        const auto& rule = grammar.rules[sit.rule_idx];

        if (sit.dot_pose < rule.rhs.size()) {
          const CharT kNextSym = rule.rhs[sit.dot_pose];

          if (grammar.is_terminal(kNextSym)) {
            const StringT kTail = rule.rhs.substr(sit.dot_pose + 1);
            auto eff_lookaheads = first_k.compute_first_k(kTail + sit.actpref);

            for (const auto& u : eff_lookaheads) {
              if (u.empty() || u[0] != kNextSym) {
                continue;
              }

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
              add_action_checked(
                  kCurrentStateId, sit.actpref,
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

  void add_action_checked(StateIdT state, const StringT& lookahead,
                          Action new_action) {
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

  /*======================= Data fields ========================*/
  BaseActionTableT action_table_;
};
}  // namespace lrk_parser::details
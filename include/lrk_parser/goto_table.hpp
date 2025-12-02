#pragma once

#include <cassert>

#include "config.hpp"
#include "first_k.hpp"
#include "grammar.hpp"
#include "situation.hpp"
#include "tables_base.hpp"

namespace lrk_parser::details {
class GotoTable {
 private:
  /*====================== Usings/Helpers ======================*/
  using BaseGotoTableT = UmapT<TransitionKey, StateIdT, TransitionKeyHash>;

 public:
  /*================= Constructors/Destructors =================*/
  GotoTable() = delete;

  GotoTable(const GotoTable&) = delete;

  GotoTable(GotoTable&&) = delete;

  GotoTable(const Grammar& grammar, const FirstK& fist_k) {
    build_goto_table(grammar, fist_k);
  }

  /*======================= Assignments ========================*/
  GotoTable& operator=(const GotoTable& /*unused*/) = delete;

  GotoTable& operator=(GotoTable&& /*unused*/) = delete;

 private:
  /*========================== Impls ===========================*/
  [[nodiscard]] details::Situations create_initial_situations(const Grammar& grammar, const FirstK& fist_k) const {
    details::Situations init_situations;

    const auto& init_rule_idxs = grammar.get_rules_idxs(Grammar::kStarSym);

    for (const auto& rule_idx : init_rule_idxs) {
      init_situations.emplace(details::Situation{
          .rule_idx = rule_idx, .dot_pose = 0, .actpref = StringT{}});
    }
    return closure(grammar, fist_k, std::move(init_situations));
  }

  [[nodiscard]] details::Situations closure(
      const Grammar& grammar, const FirstK& fist_k,
      details::Situations kernal_set) const {
    details::Situations result = std::move(kernal_set);

    DequeT<details::Situation> queue{result.begin(), result.end()};

    while (not queue.empty()) {
      auto [rule_idx, dot_pose, actpref] = queue.back();
      queue.pop_back();
      const auto& rhs = grammar.rules[rule_idx].rhs;
      if (dot_pose >= rhs.size()) {
        continue;
      }
      const auto& B = rhs[dot_pose];
      if (not grammar.is_nonterminal(B)) {
        continue;
      }
      auto beta = rhs.substr(dot_pose + 1);

      auto firsk_k = fist_k.compute_str(beta + actpref);

      for (const auto& rule_B_idx : grammar.get_rules_idxs(B)) {
        for (const auto& x : firsk_k) {
          details::Situation new_sit = {
              .rule_idx = rule_B_idx, .dot_pose = 0, .actpref = x};
          auto [it, emplace_status] = result.emplace(new_sit);
          if (emplace_status) {
            queue.emplace_front(new_sit);
          }
        }
      }
    }

    return result;
  }

  [[nodiscard]] details::Situations compute_go_situation(
      const Grammar& grammar, const FirstK& fist_k,
      const details::Situations& I, CharT X) const {
    details::Situations kernel_situations;

    for (const auto& [rule_idx, dot_pose, actpref] : I) {
      const auto& rhs = grammar.rules[rule_idx].rhs;
      if (dot_pose >= rhs.size() or X != rhs[dot_pose]) {
        continue;
      }
      kernel_situations.emplace(details::Situation{
          .rule_idx = rule_idx, .dot_pose = dot_pose + 1, .actpref = actpref});
    }

    kernel_situations = closure(grammar, fist_k, std::move(kernel_situations));

    return kernel_situations;
  }

  void build_goto_table(const Grammar& grammar, const FirstK& fist_k) {
    details::Situations I0 = create_initial_situations(grammar, fist_k);
    auto [I0_id, I0_emplace_status] = insert_sutiations(std::move(I0));
    assert(I0_emplace_status);

    DequeT<StateIdT> queue;
    queue.emplace_back(I0_id);

    while (not queue.empty()) {
      auto curr_sits_id = queue.front();
      queue.pop_front();
      auto& curr_sits = states_[curr_sits_id];

      for (const auto& X : grammar.get_all_symbols_range()) {
        auto next_sits = compute_go_situation(grammar, fist_k, curr_sits, X);
        if (next_sits.empty()) {
          continue;
        }

        auto [next_sits_id, is_next_sits_inserted] =
            insert_sutiations(std::move(next_sits));

        if (is_next_sits_inserted) {
          queue.emplace_back(next_sits_id);
        }

        TransitionKey tkey = {.current_state_id = curr_sits_id, .symbol = X};

        goto_table_.emplace(tkey, next_sits_id);
      }
    }
  }

  std::pair<StateIdT, bool> insert_sutiations(details::Situations I) {
    auto [it, emplace_status] =
        state_set_to_id_.try_emplace(std::move(I), StateIdT{});
    if (emplace_status) {
      it->second = states_.size();
      states_.emplace_back(it->first);
    }
    return {it->second, emplace_status};
  }

  /*======================= Data fields ========================*/
  BaseGotoTableT goto_table_;

  VectorT<details::Situations> states_;

  UmapT<details::Situations, StateIdT, details::SituationsHash>
      state_set_to_id_;
};
}  // namespace lrk_parser::details
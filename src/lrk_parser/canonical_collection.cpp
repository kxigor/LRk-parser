#include "lrk_parser/canonical_collection.hpp"

using BaseGotoTableT = lrk_parser::details::CanonicalCollection::BaseGotoTableT;
using StatesT = lrk_parser::details::CanonicalCollection::StatesT;
using StateSetToIdT = lrk_parser::details::CanonicalCollection::StateSetToIdT;
using Situations = lrk_parser::details::Situations;
using StateIdT = lrk_parser::details::StateIdT;

const BaseGotoTableT& lrk_parser::details::CanonicalCollection::get_goto_table()
    const noexcept {
  return goto_table_;
}

BaseGotoTableT
lrk_parser::details::CanonicalCollection::take_goto_table() noexcept {
  return std::move(goto_table_);
}

const StatesT& lrk_parser::details::CanonicalCollection::get_states()
    const noexcept {
  return states_;
}

const StateSetToIdT
lrk_parser::details::CanonicalCollection::get_state_to_id_map() const noexcept {
  return state_set_to_id_;
}

Situations lrk_parser::details::CanonicalCollection::create_initial_situations(
    const Grammar& grammar, const FirstK& fist_k) const {
  Situations init_situations;

  const auto& init_rule_idxs = grammar.get_rules_idxs(Grammar::kStarSym);

  for (const auto& rule_idx : init_rule_idxs) {
    init_situations.emplace(details::Situation{
        .rule_idx = rule_idx, .dot_pose = 0, .actpref = StringT{}});
  }
  return closure(grammar, fist_k, std::move(init_situations));
}

Situations lrk_parser::details::CanonicalCollection::closure(
    const Grammar& grammar, const FirstK& fist_k, Situations kernal_set) const {
  Situations result = std::move(kernal_set);

  DequeT<details::Situation> queue{result.begin(), result.end()};

  while (not queue.empty()) {
    auto [rule_idx, dot_pose, actpref] = queue.back();
    queue.pop_back();
    const auto& rhs = grammar.get_rule_by_idx(rule_idx).rhs;
    if (dot_pose >= rhs.size()) {
      continue;
    }
    const auto& B = rhs[dot_pose];
    if (not grammar.is_nonterminal(B)) {
      continue;
    }
    auto beta = rhs.substr(dot_pose + 1);

    auto firsk_k = fist_k.compute_first_k(beta + actpref);

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

Situations lrk_parser::details::CanonicalCollection::compute_go_situation(
    const Grammar& grammar, const FirstK& fist_k, std::size_t I_idx,
    CharT X) const {
  Situations kernel_situations;

  for (const auto& [rule_idx, dot_pose, actpref] : states_[I_idx]) {
    const auto& rhs = grammar.get_rule_by_idx(rule_idx).rhs;
    if (dot_pose >= rhs.size() or X != rhs[dot_pose]) {
      continue;
    }
    kernel_situations.emplace(details::Situation{
        .rule_idx = rule_idx, .dot_pose = dot_pose + 1, .actpref = actpref});
  }

  kernel_situations = closure(grammar, fist_k, std::move(kernel_situations));

  return kernel_situations;
}

void lrk_parser::details::CanonicalCollection::build_goto_table(
    const Grammar& grammar, const FirstK& first_k) {
  Situations I0 = create_initial_situations(grammar, first_k);
  auto [I0_id, I0_emplace_status] = insert_sutiations(std::move(I0));
  assert(I0_emplace_status);

  DequeT<StateIdT> queue;
  queue.emplace_back(I0_id);

  while (not queue.empty()) {
    auto curr_sits_id = queue.front();
    queue.pop_front();

    auto process_symbol_transition = [&](const auto& X) {
      auto next_sits = compute_go_situation(grammar, first_k, curr_sits_id, X);
      if (next_sits.empty()) {
        return;
      }

      auto [next_sits_id, is_next_sits_inserted] =
          insert_sutiations(std::move(next_sits));

      if (is_next_sits_inserted) {
        queue.emplace_back(next_sits_id);
      }

      TransitionKey tkey = {.current_state_id = curr_sits_id, .symbol = X};

      goto_table_.emplace(tkey, next_sits_id);
    };

    for (const auto& T : grammar.get_terminals()) {
      process_symbol_transition(T);
    }

    for (const auto& N : grammar.get_nonterminals()) {
      process_symbol_transition(N);
    }
  }
}

std::pair<StateIdT, bool>
lrk_parser::details::CanonicalCollection::insert_sutiations(Situations I) {
  auto [it, emplace_status] =
      state_set_to_id_.try_emplace(std::move(I), StateIdT{});
  if (emplace_status) {
    it->second = states_.size();
    states_.emplace_back(it->first);
  }
  return {it->second, emplace_status};
}

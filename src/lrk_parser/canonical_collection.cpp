#include "lrk_parser/canonical_collection.hpp"

#include <cstddef>
#include <stdexcept>
#include <utility>

#include "lrk_parser/config.hpp"
#include "lrk_parser/details/prepared_grammar.hpp"
#include "lrk_parser/first_k.hpp"
#include "lrk_parser/situation.hpp"
#include "lrk_parser/tables_base.hpp"

using CanonicalCollection = lrk_parser::details::CanonicalCollection;
using BaseGotoTableT = CanonicalCollection::BaseGotoTableT;
using StatesT = CanonicalCollection::StatesT;
using StateSetToIdT = CanonicalCollection::StateSetToIdT;
using Situations = lrk_parser::details::Situations;
using StateId = lrk_parser::details::StateId;

CanonicalCollection::CanonicalCollection(const PreparedGrammar& grammar,
                                         const FirstK& first_k) {
  CCC ctx(grammar, first_k);
  build_goto_table(ctx);
}

const BaseGotoTableT& CanonicalCollection::get_goto_table() const noexcept {
  return goto_table_;
}

BaseGotoTableT CanonicalCollection::take_goto_table() noexcept {
  return std::move(goto_table_);
}

const StatesT& CanonicalCollection::get_states() const noexcept {
  return states_;
}

const StateSetToIdT& CanonicalCollection::get_state_to_id_map() const noexcept {
  return state_set_to_id_;
}

Situations CanonicalCollection::create_initial_situations(CCC& ctx) {
  Situations init_situations{
      Situation{.rule_idx = kStartRule, .dot_pose = 0, .actpref = {}}};
  return closure(ctx, std::move(init_situations));
}

Situations CanonicalCollection::closure(CCC& ctx, Situations kernal_set) {
  Situations result = std::move(kernal_set);

  DequeT<details::Situation> queue{result.begin(), result.end()};

  while (not queue.empty()) {
    auto [rule_idx, dot_pose, actpref] = queue.back();
    queue.pop_back();
    const auto& rhs = ctx.grammar.GetRule(rule_idx).rhs;
    if (dot_pose >= rhs.size()) {
      continue;
    }
    const auto& B = rhs[dot_pose];  // NOLINT
    if (not ctx.grammar.IsNonterminal(B)) {
      continue;
    }
    auto beta = std::span{rhs}.subspan(dot_pose + 1);

    auto firsk_k = ctx.first_k.compute_first_k(beta, actpref);

    for (const auto& rule_B_idx : ctx.grammar.RulesFor(B)) {
      for (const auto& x : firsk_k) {
        const details::Situation kNewSit = {
            .rule_idx = rule_B_idx, .dot_pose = 0, .actpref = x};
        auto [it, emplace_status] = result.emplace(kNewSit);
        if (emplace_status) {
          queue.emplace_front(kNewSit);
        }
      }
    }
  }

  return result;
}

Situations CanonicalCollection::compute_go_situation(CCC& ctx,
                                                     StateId state_idx,
                                                     SymbolId sym) const {
  Situations kernel_situations;

  for (const auto& [rule_idx, dot_pose, actpref] :
       states_[std::to_underlying(state_idx)]) {
    const auto& rhs = ctx.grammar.GetRule(rule_idx).rhs;
    if (dot_pose >= rhs.size() or sym != rhs[dot_pose]) {
      continue;
    }
    kernel_situations.emplace(details::Situation{
        .rule_idx = rule_idx, .dot_pose = dot_pose + 1, .actpref = actpref});
  }

  kernel_situations = closure(ctx, std::move(kernel_situations));

  return kernel_situations;
}

void CanonicalCollection::build_goto_table(CCC& ctx) {
  Situations I0 = create_initial_situations(ctx);
  auto [I0_id, I0_emplace_status] = insert_sutiations(std::move(I0));
  if (not I0_emplace_status) {
    throw std::logic_error("Failed to initialize state");
  }

  DequeT<StateId> queue;
  queue.emplace_back(I0_id);

  while (not queue.empty()) {
    auto curr_sits_id = queue.front();
    queue.pop_front();

    auto process_symbol_transition = [&](const auto& X) {
      auto next_sits = compute_go_situation(ctx, curr_sits_id, X);
      if (next_sits.empty()) {
        return;
      }

      auto [next_sits_id, is_next_sits_inserted] =
          insert_sutiations(std::move(next_sits));

      if (is_next_sits_inserted) {
        queue.emplace_back(next_sits_id);
      }

      const auto kTKey =
          TransitionKey{.current_state_id = curr_sits_id, .symbol = X};

      goto_table_.emplace(kTKey, next_sits_id);
    };

    for (const auto& T : ctx.grammar.Terminals()) {
      process_symbol_transition(T);
    }

    for (const auto& N : ctx.grammar.Nonterminals()) {
      process_symbol_transition(N);
    }
  }
}

std::pair<StateId, bool> CanonicalCollection::insert_sutiations(
    Situations state) {
  auto [it, emplace_status] =
      state_set_to_id_.try_emplace(std::move(state), StateId{});
  if (emplace_status) {
    it->second = StateId{states_.size()};
    states_.emplace_back(it->first);
  }
  return {it->second, emplace_status};
}

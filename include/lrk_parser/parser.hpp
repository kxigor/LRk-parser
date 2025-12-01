#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <deque>
#include <format>
#include <functional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "config.hpp"
#include "grammar.hpp"
#include "rule.hpp"

namespace lrk_parser {
class LrkParser {
 public:
  /*====================== Usings/Helpers ======================*/


  struct Situation {
    [[nodiscard]] bool operator==(const Situation& /*unused*/) const noexcept =
        default;

    std::size_t rule_idx{};
    std::size_t dot_pose{};
    StringT actpref{};
  };

  struct SituationHash {
    /*=========== Hash ===========*/
    [[nodiscard]] std::size_t operator()(const Situation& sit) const noexcept {
      // NOLINTBEGIN
      std::size_t seed = 0;
      seed ^= sit.rule_idx + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      seed ^= sit.dot_pose + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      seed ^= std::hash<StringT>{}(sit.actpref) + 0x9e3779b9 + (seed << 6) +
              (seed >> 2);
      // NOLINTEND
      return seed;
    }
  };

  using Situations = UsetT<Situation, SituationHash>;

  struct SituationsHash {
    [[nodiscard]] std::size_t operator()(const Situations& set) const noexcept {
      // NOLINTBEGIN
      std::size_t seed = set.size();

      SituationHash situation_hasher;

      for (const auto& situation : set) {
        std::size_t situation_hash = situation_hasher(situation);
        seed ^= situation_hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      }
      // NOLINTEND
      return seed;
    }
  };





  using StateIdT = std::size_t;

  struct TransitionKey {
    [[nodiscard]] bool operator==(const TransitionKey& other) const = default;

    StateIdT current_state_id;
    CharT symbol;
  };

  struct TransitionKeyHash {
    [[nodiscard]] std::size_t operator()(
        const TransitionKey& tkey) const noexcept {
      // NOLINTBEGIN
      std::size_t seed = 0;
      seed ^= tkey.current_state_id + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      seed ^= static_cast<std::size_t>(tkey.symbol) + 0x9e3779b9 + (seed << 6) +
              (seed >> 2);
      // NOLINTEND
      return seed;
    }
  };

  enum class ActionType { Error, Shift, Reduce, Accept };

  struct Action {
    [[nodiscard]] bool operator==(const Action& other) const = default;

    ActionType type = ActionType::Error;
    std::size_t value = 0;
  };

  struct ActionKey {
    [[nodiscard]] bool operator==(const ActionKey& other) const = default;

    StateIdT state_id;
    StringT lookahead;
  };

  struct ActionKeyHash {
    [[nodiscard]] std::size_t operator()(const ActionKey& key) const noexcept {
      // NOLINTBEGIN
      std::size_t seed = 0;
      seed ^= std::hash<StateIdT>{}(key.state_id) + 0x9e3779b9 + (seed << 6) +
              (seed >> 2);
      for (auto c : key.lookahead) {
        seed ^= std::hash<CharT>{}(c) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      }
      // NOLINTEND
      return seed;
    }
  };

  using GotoTableT = UmapT<TransitionKey, StateIdT, TransitionKeyHash>;

  using ActionTableT = UmapT<ActionKey, Action, ActionKeyHash>;

  /*================= Consturctors/Destructors =================*/
  LrkParser() noexcept = default;

  LrkParser(const LrkParser& /*unused*/) = default;

  LrkParser(LrkParser&& /*unused*/) noexcept = default;

  ~LrkParser() noexcept = default;

  /*======================= Assignments ========================*/
  LrkParser& operator=(const LrkParser& /*unused*/) = default;

  LrkParser& operator=(LrkParser&& /*unused*/) noexcept = default;

  /*===================== Parser Interface =====================*/
  void fit(Grammar grammar, std::size_t k) {
    grammar_ = std::move(grammar);
    k_ = k;

    initialize_first_k_sets();
    compute_first_k_fixed_point();
    build_goto_table();
    build_action_table();
  }

  [[nodiscard]] bool predict(const StringT& word) {
    std::vector<StateIdT> stack;
    stack.reserve(word.size());
    stack.push_back(0);

    std::size_t cursor = 0;

    volatile bool b = true;
    while (b) {
      StateIdT current_state = stack.back();

      StringT u;
      if (cursor < word.size()) {
        u = word.substr(cursor, k_);
      }

      ActionKey key{current_state, u};

      if (!action_table_.contains(key)) {
        return false;
      }

      Action action = action_table_.at(key);

      if (action.type == ActionType::Shift) {
        if (cursor >= word.size()) {
          return false;
        }

        stack.push_back(action.value);

        ++cursor;
      } else if (action.type == ActionType::Reduce) {
        const auto& rule = grammar_.rules[action.value];

        std::size_t symbols_to_pop = rule.rhs.size();

        if (stack.size() < symbols_to_pop + 1) {
          return false;
        }

        for (std::size_t i = 0; i < symbols_to_pop; ++i) {
          stack.pop_back();
        }

        StateIdT state_top = stack.back();

        TransitionKey goto_key{state_top, rule.lhs};

        if (!goto_table_.contains(goto_key)) {
          return false;
        }

        StateIdT next_state = goto_table_.at(goto_key);

        stack.push_back(next_state);

      } else if (action.type == ActionType::Accept) {
        return cursor == word.size();
      } else {
        return false;
      }
    }

    return false;
  }

 private:
  /*========================== Impls ===========================*/
  void initialize_first_k_sets() {
    for (const auto& terminal : grammar_.terminals) {
      first_k_[terminal] = {{terminal}};
    }
  }

  void compute_first_k_fixed_point() {
    while (update_first_k_in_single_iteration());
  }

  bool update_first_k_in_single_iteration() {
    bool changed = false;
    for (const auto& rule : grammar_.rules) {
      auto rhs_first_k = compute_first_k_for_rhs(rule);
      changed |= update_lhs_first_k_if_changed(rule, rhs_first_k);
    }
    return changed;
  }

  [[nodiscard]] UsetT<StringT> compute_first_k_for_rhs(const Rule& rule) {
    UsetT<StringT> rhs_first_k_result = {StringT{}};

    for (const auto& sym : rule.rhs) {
      auto sym_first_k = first_k_[sym];
      rhs_first_k_result = concat_k_sets(rhs_first_k_result, sym_first_k);
    }

    return rhs_first_k_result;
  }

  bool update_lhs_first_k_if_changed(const Rule& rule,
                                     const UsetT<StringT>& rhs_first_k) {
    auto& lhs_first_k = first_k_[rule.lhs];
    const auto kSizeBefore = lhs_first_k.size();
    union_k_sets(lhs_first_k, rhs_first_k);
    const auto kSizeAfter = lhs_first_k.size();
    return kSizeBefore != kSizeAfter;
  }

  [[nodiscard]] UsetT<StringT> concat_k_sets(
      const UsetT<StringT>& lhs_set, const UsetT<StringT>& rhs_set) const {
    UsetT<StringT> result;
    for (const auto& lhs : lhs_set) {
      if (lhs.size() >= k_) {
        result.emplace(lhs.substr(0, k_));
        continue;
      }
      for (const auto& rhs : rhs_set) {
        StringT added = lhs + rhs;
        if (added.size() > k_) {
          added.resize(k_);
        }
        result.emplace(std::move(added));
      }
    }
    return result;
  }

  void union_k_sets(UsetT<StringT>& lhs_set,
                    const UsetT<StringT>& rhs_set) const {
    for (const auto& rhs : rhs_set) {
      lhs_set.emplace(rhs);
    }
  }

  [[nodiscard]] UsetT<StringT> compute_first_k_of_str(
      const StringT& str) const {
    UsetT<StringT> result = {StringT{}};
    for (const auto& sym : str) {
      result = concat_k_sets(result, first_k_.at(sym));
    }
    return result;
  }

  [[nodiscard]] Situations create_initial_situations() const {
    Situations init_situations;

    const auto& init_rule_idxs = grammar_.get_rules_idxs(Grammar::kStarSym);

    for (const auto& rule_idx : init_rule_idxs) {
      init_situations.emplace(
          Situation{.rule_idx = rule_idx, .dot_pose = 0, .actpref = StringT{}});
    }
    return closure(std::move(init_situations));
  }

  [[nodiscard]] Situations closure(Situations kernal_set) const {
    Situations result = std::move(kernal_set);

    DequeT<Situation> queue{result.begin(), result.end()};

    while (not queue.empty()) {
      auto [rule_idx, dot_pose, actpref] = queue.back();
      queue.pop_back();
      const auto& rhs = grammar_.rules[rule_idx].rhs;
      if (dot_pose >= rhs.size()) {
        continue;
      }
      const auto& B = rhs[dot_pose];
      if (not grammar_.is_nonterminal(B)) {
        continue;
      }
      auto beta = rhs.substr(dot_pose + 1);

      auto firsk_k = compute_first_k_of_str(beta + actpref);

      for (const auto& rule_B_idx : grammar_.get_rules_idxs(B)) {
        for (const auto& x : firsk_k) {
          Situation new_sit = {
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

  [[nodiscard]] Situations compute_go_situation(const Situations& I,
                                                CharT X) const {
    Situations kernel_situations;

    for (const auto& [rule_idx, dot_pose, actpref] : I) {
      const auto& rhs = grammar_.rules[rule_idx].rhs;
      if (dot_pose >= rhs.size() or X != rhs[dot_pose]) {
        continue;
      }
      kernel_situations.emplace(Situation{
          .rule_idx = rule_idx, .dot_pose = dot_pose + 1, .actpref = actpref});
    }

    kernel_situations = closure(std::move(kernel_situations));

    return kernel_situations;
  }

  void build_goto_table() {
    Situations I0 = create_initial_situations();
    auto [I0_id, I0_emplace_status] = insert_sutiations(std::move(I0));
    assert(I0_emplace_status);

    DequeT<StateIdT> queue;
    queue.emplace_back(I0_id);

    while (not queue.empty()) {
      auto curr_sits_id = queue.front();
      queue.pop_front();
      auto& curr_sits = states_[curr_sits_id];

      for (const auto& X : grammar_.get_all_symbols_range()) {
        auto next_sits = compute_go_situation(curr_sits, X);
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

  std::pair<StateIdT, bool> insert_sutiations(Situations I) {
    auto [it, emplace_status] =
        state_set_to_id_.try_emplace(std::move(I), StateIdT{});
    if (emplace_status) {
      it->second = states_.size();
      states_.emplace_back(it->first);
    }
    return {it->second, emplace_status};
  }

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

  /*======================= Data fields ========================*/
  Grammar grammar_;
  VectorT<Situations> states_;
  UmapT<Situations, StateIdT, SituationsHash> state_set_to_id_;

  GotoTableT goto_table_;
  ActionTableT action_table_;

  std::size_t k_{};
  UmapT<CharT, UsetT<StringT>> first_k_{};
};
}  // namespace lrk_parser
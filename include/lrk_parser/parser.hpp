#pragma once

#include <cassert>
#include <cstddef>
#include <utility>

#include "action_table.hpp"
#include "canonical_collection.hpp"
#include "config.hpp"
#include "first_k.hpp"
#include "goto_table.hpp"
#include "grammar.hpp"
#include "rule.hpp"
#include "tables_base.hpp"

namespace lrk_parser {
class LrkParser {
  /*================= Consturctors/Destructors =================*/
 public:
  LrkParser() noexcept = default;

  LrkParser(const LrkParser& /*unused*/) = default;

  LrkParser(LrkParser&& /*unused*/) noexcept = default;

  ~LrkParser() noexcept = default;

  /*======================= Assignments ========================*/
  LrkParser& operator=(const LrkParser& /*unused*/) = default;

  LrkParser& operator=(LrkParser&& /*unused*/) noexcept = default;

  /*===================== Parser Interface =====================*/
  void fit(Grammar grammar, std::size_t k) {
    k_ = k;
    grammar_ = std::move(grammar);

    const details::FirstK kFirstK(grammar_, k_);
    details::CanonicalCollection lr_collection(grammar_, kFirstK);

    action_table_ = details::ActionTable(grammar_, kFirstK, lr_collection);
    goto_table_ = details::GotoTable(std::move(lr_collection));
  }

  [[nodiscard]] bool predict(const StringT& word) const {
    VectorT<details::StateIdT> stack;
    stack.reserve(word.size());
    stack.push_back(0);

    std::size_t cursor = 0;

    const volatile bool kB = true;
    while (kB) {
      const details::StateIdT kCurrentState = stack.back();

      StringT u;
      if (cursor < word.size()) {
        u = word.substr(cursor, k_);
      }

      const details::ActionKey kAKey{.state_id = kCurrentState, .lookahead = u};

      if (not action_table_.has_parse_action(kAKey)) {
        return false;
      }

      const details::Action kAction = action_table_.get_parse_action(kAKey);

      if (kAction.type == details::ActionType::Shift) {
        if (cursor >= word.size()) {
          return false;
        }

        stack.push_back(kAction.value);

        ++cursor;
      } else if (kAction.type == details::ActionType::Reduce) {
        const auto& rule = grammar_.rules[kAction.value];

        const std::size_t kSymsToPop = rule.rhs.size();

        if (stack.size() < kSymsToPop + 1) {
          return false;
        }

        for (std::size_t i = 0; i < kSymsToPop; ++i) {
          stack.pop_back();
        }

        const details::StateIdT kStateTop = stack.back();

        const details::TransitionKey kGotoKey{.current_state_id = kStateTop,
                                              .symbol = rule.lhs};

        if (not goto_table_.has_goto_state(kGotoKey)) {
          return false;
        }

        const details::StateIdT kNextState =
            goto_table_.get_goto_state(kGotoKey);

        stack.push_back(kNextState);

      } else if (kAction.type == details::ActionType::Accept) {
        return cursor == word.size();
      } else {
        return false;
      }
    }

    return false;
  }

  /*======================= Data fields ========================*/
 private:
  std::size_t k_{};
  details::Grammar grammar_;
  details::GotoTable goto_table_;
  details::ActionTable action_table_;
};
}  // namespace lrk_parser
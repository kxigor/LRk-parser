#include "lrk_parser/parser.hpp"

#include <cstddef>
#include <utility>

#include "lrk_parser/canonical_collection.hpp"
#include "lrk_parser/config.hpp"
#include "lrk_parser/first_k.hpp"
#include "lrk_parser/grammar.hpp"
#include "lrk_parser/tables_base.hpp"

void lrk_parser::LrkParser::fit(Grammar grammar, std::size_t k) {
  k_ = k;
  grammar_ = std::move(grammar);

  const details::FirstK kFirstK(grammar_, k_);
  details::CanonicalCollection lr_collection(grammar_, kFirstK);

  action_table_ = details::ActionTable(grammar_, kFirstK, lr_collection);
  goto_table_ = details::GotoTable(std::move(lr_collection));
}

bool lrk_parser::LrkParser::predict(const StringT& word) const {
  VectorT<details::StateIdT> stack;
  stack.reserve(word.size());
  stack.push_back(0);

  std::size_t cursor = 0;

  const volatile bool kB = true;
  while (kB) {
    const auto kCurrentStateId = stack.back();

    StringT u;
    if (cursor < word.size()) {
      u = word.substr(cursor, k_);
    }

    const auto kAKey =
        details::ActionKey{.state_id = kCurrentStateId, .lookahead = u};

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
      const auto& rule = grammar_.get_rule_by_idx(kAction.value);

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

      const details::StateIdT kNextState = goto_table_.get_goto_state(kGotoKey);

      stack.push_back(kNextState);

    } else if (kAction.type == details::ActionType::Accept) {
      return cursor == word.size();
    } else {
      return false;
    }
  }

  std::unreachable();
}
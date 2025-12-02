#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "config.hpp"
#include "first_k.hpp"
#include "grammar.hpp"
#include "rule.hpp"
#include "situation.hpp"
#include "goto_table.hpp"
#include "action_table.hpp"
#include "tables_base.hpp"

namespace lrk_parser {
class LrkParser {
 public:
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

    details::FirstK first_k(grammar_, k_);
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

  /*======================= Data fields ========================*/
  Grammar grammar_;

  std::size_t k_{};
};
}  // namespace lrk_parser
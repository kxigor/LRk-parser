#pragma once

#include <cassert>
#include <cstddef>
#include <format>
#include <ostream>

#include "action_table.hpp"
#include "config.hpp"
#include "goto_table.hpp"
#include "grammar.hpp"
#include "tables_base.hpp"

namespace lrk_parser {
class LrkParser {
  struct PredictContext {
    explicit PredictContext(const StringT& word) : word(word) {
      stack.reserve(word.size());
      stack.push_back(details::StateId{0});
    }

    StringT get_lookahead(std::size_t k) { return word.substr(cursor, k); }

    bool is_word_recognized{false};
    bool is_processing_word{true};

    std::size_t cursor{0};
    VectorT<details::StateId> stack;

    const StringT& word;  // NOLINT
  };

  /*================= Consturctors/Destructors =================*/
 public:
  LrkParser() noexcept = default;

  LrkParser(const LrkParser& /*unused*/) = default;

  LrkParser(LrkParser&& /*unused*/) noexcept = default;

  ~LrkParser() noexcept = default;

  /*======================= Assignments ========================*/
  LrkParser& operator=(const LrkParser& /*unused*/) = default;

  LrkParser& operator=(LrkParser&& /*unused*/) noexcept = default;

  /*========================== Output ==========================*/
  friend struct std::formatter<LrkParser>;

  friend std::ostream& operator<<(std::ostream& os, const LrkParser& parser);

  /*===================== Parser Interface =====================*/
  void fit(const Grammar& grammar, std::size_t k);

  [[nodiscard]] bool predict(const StringT& word) const;

 private:
  /*=========================== Imls ===========================*/
  void fit_impl(std::size_t k);

  OptionalT<details::Action> get_next_action(PredictContext& ctx) const;
  static void handle_shift_case(PredictContext& ctx, std::size_t next_state_id);
  void handle_reduce_case(PredictContext& ctx, std::size_t next_state_id) const;
  static void handle_accept_case(PredictContext& ctx);
  static void handle_error_case(PredictContext& ctx);

  /*======================= Data fields ========================*/
  std::size_t k_{};
  OptionalT<details::PreparedGrammar> grammar_;
  details::GotoTable goto_table_;
  details::ActionTable action_table_;
};
}  // namespace lrk_parser

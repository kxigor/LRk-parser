#pragma once

#include <cassert>
#include <cstddef>
#include <format>

#include "action_table.hpp"
#include "config.hpp"
#include "goto_table.hpp"
#include "grammar.hpp"

namespace lrk_parser {
class LrkParser {
  using ActionType = details::ActionType;

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

  /*===================== Parser Interface =====================*/
  void fit(Grammar grammar, std::size_t k);

  [[nodiscard]] bool predict(const StringT& word) const;

 private:
  /*======================= Data fields ========================*/
  std::size_t k_{};
  details::Grammar grammar_;
  details::GotoTable goto_table_;
  details::ActionTable action_table_;
};
}  // namespace lrk_parser
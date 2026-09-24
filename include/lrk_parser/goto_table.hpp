#pragma once

#include <format>

#include "details/prepared_grammar.hpp"
#include "tables_base.hpp"

namespace lrk_parser::details {
class GotoTable {
 public:
  GotoTable() = default;

  [[nodiscard]] static GotoTable Build(const PreparedGrammar& grammar,
                                       TransitionMap transitions);

  friend struct std::formatter<GotoTable>;

  [[nodiscard]] bool HasGotoState(const TransitionKey& t_key) const;

  [[nodiscard]] const StateId& GetGotoState(const TransitionKey& t_key) const;

 private:
  explicit GotoTable(TransitionMap transitions);

  TransitionMap goto_table_;
};
}  // namespace lrk_parser::details

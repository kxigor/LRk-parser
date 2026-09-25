#pragma once

#include <iosfwd>

#include "prepared_grammar.hpp"
#include "tables_base.hpp"

namespace lrk_parser::details {
class GotoTable {
 public:
  GotoTable() = default;

  [[nodiscard]] static GotoTable Build(const PreparedGrammar& grammar,
                                       TransitionMap transitions);

  friend std::ostream& operator<<(std::ostream& os, const GotoTable& table);

  [[nodiscard]] const StateId* FindState(const TransitionKey& key) const;

 private:
  explicit GotoTable(TransitionMap transitions);

  TransitionMap goto_table_;
};
}  // namespace lrk_parser::details

#pragma once

#include <cassert>
#include <format>

#include "canonical_collection.hpp"
#include "tables_base.hpp"

namespace lrk_parser::details {
class GotoTable {
  using BaseGotoTableT = CanonicalCollection::TransitionMap;

 public:
  GotoTable() = default;

  explicit GotoTable(const CanonicalCollection& lr_colletion);

  explicit GotoTable(CanonicalCollection&& lr_colletion);

  GotoTable(const GotoTable& /*unused*/) = default;

  GotoTable(GotoTable&& /*unused*/) = default;

  ~GotoTable() = default;

  GotoTable& operator=(const GotoTable& /*unused*/) = default;

  GotoTable& operator=(GotoTable&& /*unused*/) = default;

  friend struct std::formatter<GotoTable>;

  [[nodiscard]] bool has_goto_state(const TransitionKey& t_key) const;

  [[nodiscard]] const StateId& get_goto_state(const TransitionKey& t_key) const;

 private:
  BaseGotoTableT goto_table_;
};
}  // namespace lrk_parser::details

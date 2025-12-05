#pragma once

#include <cassert>
#include <utility>

#include "canonical_collection.hpp"
#include "tables_base.hpp"

namespace lrk_parser::details {
class GotoTable {
  /*====================== Usings/Helpers ======================*/
  using BaseGotoTableT = CanonicalCollection::BaseGotoTableT;

  /*================= Constructors/Destructors =================*/
 public:
  GotoTable() = default;

  explicit GotoTable(const CanonicalCollection& lr_colletion)
      : goto_table_(lr_colletion.get_goto_table()) {}

  explicit GotoTable(CanonicalCollection&& lr_colletion)
      : goto_table_(std::move(lr_colletion).take_goto_table()) {}

  GotoTable(const GotoTable& /*unused*/) = default;

  GotoTable(GotoTable&& /*unused*/) = default;

  ~GotoTable() = default;

  /*======================= Assignments ========================*/
  GotoTable& operator=(const GotoTable& /*unused*/) = default;

  GotoTable& operator=(GotoTable&& /*unused*/) = default;

  /*===================== Table Operations =====================*/
  [[nodiscard]] bool has_goto_state(const TransitionKey& t_key) const;

  [[nodiscard]] const StateIdT& get_goto_state(
      const TransitionKey& t_key) const;

  /*======================= Data Fields ========================*/
 private:
  BaseGotoTableT goto_table_;
};
}  // namespace lrk_parser::details
#pragma once

#include <cstddef>
#include <format>
#include <stdexcept>

#include "canonical_collection.hpp"
#include "config.hpp"
#include "first_k.hpp"
#include "grammar.hpp"
#include "tables_base.hpp"

namespace lrk_parser::details {
class ActionTable {
  /*====================== Usings/Helpers ======================*/
  using BaseActionTableT = UmapT<ActionKey, Action, ActionKeyHash>;

  friend std::ostream& operator<<(std::ostream& os, const ActionTable& table);

  /*================= Constructors/Destructors =================*/
 public:
  ActionTable() = default;

  ActionTable(const Grammar& grammar, const FirstK& first_k,
              const CanonicalCollection& lr_collection);

  ActionTable(const ActionTable& /*unused*/) = default;

  ActionTable(ActionTable&& /*unused*/) = default;

  ~ActionTable() = default;

  /*======================= Assignments ========================*/
  ActionTable& operator=(const ActionTable& /*unused*/) = default;

  ActionTable& operator=(ActionTable&& /*unused*/) = default;

  /*===================== Table Operations =====================*/
  [[nodiscard]] bool has_parse_action(const ActionKey& a_key) const;

  [[nodiscard]] const details::Action& get_parse_action(
      const ActionKey& a_key) const;

  /*========================== Impls ===========================*/
 private:
  void build_action_table(const Grammar& grammar, const FirstK& first_k,
                          const CanonicalCollection& lr_collection);

  void add_action_checked(StateIdT state, const StringT& lookahead,
                          Action new_action);

  /*======================= Data fields ========================*/
  BaseActionTableT action_table_;
};
}  // namespace lrk_parser::details
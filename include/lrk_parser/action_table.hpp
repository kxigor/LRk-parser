#pragma once

#include <cstddef>
#include <format>
#include <ostream>

#include "canonical_collection.hpp"
#include "config.hpp"
#include "first_k.hpp"
#include "grammar.hpp"
#include "rule.hpp"
#include "situation.hpp"
#include "tables_base.hpp"

namespace lrk_parser::details {
class ActionTable {
  /*====================== Usings/Helpers ======================*/
  using BaseActionTableT = UmapT<ActionKey, Action, ActionKeyHash>;

  // NOLINTBEGIN
  struct ActionTableContext {
    ActionTableContext(const Grammar& grammar, const FirstK& first_k,
                       const CanonicalCollection& lr_collection)
        : grammar(grammar), first_k(first_k), lr_collection(lr_collection) {}

    ~ActionTableContext() = default;

    const Grammar& grammar;
    const FirstK& first_k;
    const CanonicalCollection& lr_collection;
  };
  // NOLINTEND

  using ATC = ActionTableContext;

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

  /*========================== Output ==========================*/
  friend struct std::formatter<ActionTable>;

  friend std::ostream& operator<<(std::ostream& os, const ActionTable& table);

  /*===================== Table Operations =====================*/
  [[nodiscard]] bool has_parse_action(const ActionKey& a_key) const;

  [[nodiscard]] const details::Action& get_parse_action(
      const ActionKey& a_key) const;

  /*========================== Impls ===========================*/
 private:
  void build_action_table(ATC& ctx);

  void process_state_situations(ATC& ctx, std::size_t state_idx);

  void handle_shift_insert(ATC& ctx, std::size_t state_idx,
                           const Situation& sit, const Rule& rule);

  void handle_accept_insert(std::size_t state_idx, const Situation& sit);

  void handle_reduce_insert(std::size_t state_idx, const Situation& sit);

  void add_action_checked(StateIdT state, const StringT& lookahead,
                          Action new_action);

  /*======================= Data fields ========================*/
  BaseActionTableT action_table_;
};
}  // namespace lrk_parser::details
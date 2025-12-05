#pragma once

#include <cassert>

#include "config.hpp"
#include "first_k.hpp"
#include "grammar.hpp"
#include "situation.hpp"
#include "tables_base.hpp"

namespace lrk_parser::details {
class CanonicalCollection {
  /*========================= Friends ==========================*/
#ifdef UNIT_TESTS
  friend class CanonicalCollectionTestAccessor;
#endif

  friend std::ostream& operator<<(std::ostream& os,
                                  const CanonicalCollection& cc);

  /*====================== Usings/Helpers ======================*/
 public:
  using BaseGotoTableT = UmapT<TransitionKey, StateIdT, TransitionKeyHash>;
  using StatesT = VectorT<details::Situations>;
  using StateSetToIdT =
      UmapT<details::Situations, StateIdT, details::SituationsHash>;

  /*================= Constructors/Destructors =================*/
  CanonicalCollection() = delete;

  CanonicalCollection(const CanonicalCollection&) = default;

  CanonicalCollection(CanonicalCollection&&) = default;

  CanonicalCollection(const Grammar& grammar, const FirstK& fist_k);

  /*======================= Assignments ========================*/
  CanonicalCollection& operator=(const CanonicalCollection& /*unused*/) =
      default;

  CanonicalCollection& operator=(CanonicalCollection&& /*unused*/) = default;

  /*========================= Getters ==========================*/
  [[nodiscard]] const BaseGotoTableT& get_goto_table() const noexcept;

  BaseGotoTableT take_goto_table() noexcept;

  [[nodiscard]] const StatesT& get_states() const noexcept;

  [[nodiscard]] const StateSetToIdT get_state_to_id_map() const noexcept;

  /*========================== Impls ===========================*/
 private:
  [[nodiscard]] details::Situations create_initial_situations(
      const Grammar& grammar, const FirstK& fist_k) const;

  [[nodiscard]] details::Situations closure(
      const Grammar& grammar, const FirstK& fist_k,
      details::Situations kernal_set) const;

  [[nodiscard]] details::Situations compute_go_situation(const Grammar& grammar,
                                                         const FirstK& fist_k,
                                                         std::size_t I_idx,
                                                         CharT X) const;

  void build_goto_table(const Grammar& grammar, const FirstK& first_k);

  std::pair<StateIdT, bool> insert_sutiations(details::Situations I);

  /*======================= Data fields ========================*/
  StatesT states_;
  StateSetToIdT state_set_to_id_;

  BaseGotoTableT goto_table_;
};
}  // namespace lrk_parser::details
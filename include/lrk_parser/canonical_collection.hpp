#pragma once

#include <cassert>
#include <cstddef>
#include <ostream>
#include <utility>

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
  // NOLINTBEGIN
  struct CanonicalCollectionContext {
    CanonicalCollectionContext(const Grammar& grammar, const FirstK& first_k)
        : grammar(grammar), first_k(first_k) {}

    const Grammar& grammar;
    const FirstK& first_k;
  };
  // NOLINTEND

  using CCC = CanonicalCollectionContext;

  using Situations = details::Situations;

 public:
  using BaseGotoTableT = UmapT<TransitionKey, StateIdT, TransitionKeyHash>;
  using StatesT = VectorT<Situations>;
  using StateSetToIdT = UmapT<Situations, StateIdT, details::SituationsHash>;

  /*================= Constructors/Destructors =================*/
  CanonicalCollection() = delete;

  CanonicalCollection(const CanonicalCollection&) = default;

  CanonicalCollection(CanonicalCollection&&) = default;

  CanonicalCollection(const Grammar& grammar, const FirstK& first_k);

  ~CanonicalCollection() = default;

  /*======================= Assignments ========================*/
  CanonicalCollection& operator=(const CanonicalCollection& /*unused*/) =
      default;

  CanonicalCollection& operator=(CanonicalCollection&& /*unused*/) = default;

  /*========================= Getters ==========================*/
  [[nodiscard]] const BaseGotoTableT& get_goto_table() const noexcept;

  BaseGotoTableT take_goto_table() noexcept;

  [[nodiscard]] const StatesT& get_states() const noexcept;

  [[nodiscard]] const StateSetToIdT& get_state_to_id_map() const noexcept;

  /*========================== Impls ===========================*/
 private:
  [[nodiscard]] static Situations create_initial_situations(CCC& ctx);

  [[nodiscard]] static Situations closure(CCC& ctx, Situations kernal_set);

  [[nodiscard]] Situations compute_go_situation(CCC& ctx, std::size_t state_idx,
                                                CharT sym) const;

  void build_goto_table(CCC& ctx);

  std::pair<StateIdT, bool> insert_sutiations(Situations state);

  /*======================= Data fields ========================*/
  StatesT states_;
  StateSetToIdT state_set_to_id_;

  BaseGotoTableT goto_table_;
};
}  // namespace lrk_parser::details
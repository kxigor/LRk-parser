#pragma once

#include <cstddef>
#include <format>
#include <ostream>

#include "config.hpp"
#include "details/prepared_grammar.hpp"

namespace lrk_parser::details {
class FirstK {
  /*========================= Friends ==========================*/
#ifdef UNIT_TESTS
  friend class FirstKTestAccessor;
#endif

  /*================= Constructors/Destructors =================*/
 public:
  FirstK() = delete;

  FirstK(const FirstK& /*unused*/) = default;

  FirstK(FirstK&& /*unused*/) = default;

  FirstK(const PreparedGrammar& grammar, std::size_t k);

  ~FirstK() = default;

  /*======================= Assignments ========================*/
  FirstK& operator=(const FirstK& /*unused*/) = default;

  FirstK& operator=(FirstK&& /*unused*/) = default;

  /*========================== Output ==========================*/
  friend struct std::formatter<FirstK>;

  friend std::ostream& operator<<(std::ostream& os, const FirstK& first_k_obj);

  /*==================== First_k Computation ===================*/
  [[nodiscard]] UsetT<StringT> compute_first_k(
      std::span<const SymbolId> str, const StringT& lookahead = {}) const;

  /*========================== Impls ===========================*/
 private:
  void initialize_first_k_sets(const PreparedGrammar& grammar);

  void compute_first_k_fixed_point(const PreparedGrammar& grammar);

  bool update_first_k_in_single_iteration(const PreparedGrammar& grammar);

  [[nodiscard]] UsetT<StringT> compute_first_k_for_rhs(
      const details::PreparedRule& rule);

  bool update_lhs_first_k_if_changed(const details::PreparedRule& rule,
                                     const UsetT<StringT>& rhs_first_k);

  [[nodiscard]] UsetT<StringT> concat_k_sets(
      const UsetT<StringT>& lhs_set, const UsetT<StringT>& rhs_set) const;

  static void union_k_sets(UsetT<StringT>& lhs_set,
                           const UsetT<StringT>& rhs_set);

  /*======================= Data Fields ========================*/
  std::size_t k_{};
  UmapT<SymbolId, UsetT<StringT>> first_k_;
};

}  // namespace lrk_parser::details
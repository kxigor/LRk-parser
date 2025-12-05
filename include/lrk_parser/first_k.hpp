#pragma once

#include <cstddef>
#include <utility>

#include "config.hpp"
#include "grammar.hpp"
#include "rule.hpp"

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

  FirstK(const Grammar& grammar, std::size_t k);

  ~FirstK() = default;

  /*======================= Assignments ========================*/
  FirstK& operator=(const FirstK& /*unused*/) = default;

  FirstK& operator=(FirstK&& /*unused*/) = default;

  /*==================== First_k Computation ===================*/
  [[nodiscard]] UsetT<StringT> compute_first_k(const StringT& str) const;

  /*========================== Impls ===========================*/
 private:
  void initialize_first_k_sets(const Grammar& grammar);

  void compute_first_k_fixed_point(const Grammar& grammar);

  bool update_first_k_in_single_iteration(const Grammar& grammar);

  [[nodiscard]] UsetT<StringT> compute_first_k_for_rhs(
      const details::Rule& rule);

  bool update_lhs_first_k_if_changed(const details::Rule& rule,
                                     const UsetT<StringT>& rhs_first_k);

  [[nodiscard]] UsetT<StringT> concat_k_sets(
      const UsetT<StringT>& lhs_set, const UsetT<StringT>& rhs_set) const;

  static void union_k_sets(UsetT<StringT>& lhs_set,
                           const UsetT<StringT>& rhs_set);

  friend std::ostream& operator<<(std::ostream& os, const FirstK& first_k_obj);

  /*======================= Data Fields ========================*/
  std::size_t k_{};
  UmapT<CharT, UsetT<StringT>> first_k_;
};

}  // namespace lrk_parser::details
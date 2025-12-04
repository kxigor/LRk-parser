#pragma once

#include <cstddef>
#include <utility>

#include "config.hpp"
#include "grammar.hpp"
#include "rule.hpp"

namespace lrk_parser::details {
class FirstK {
 public:
  /*================= Constructors/Destructors =================*/
  FirstK() = delete;

  FirstK(const FirstK& /*unused*/) = default;

  FirstK(FirstK&& /*unused*/) = default;

  FirstK(const Grammar& grammar, const std::size_t& k) : k_(k) {
    initialize_first_k_sets(grammar);
    compute_first_k_fixed_point(grammar);
  }

  ~FirstK() = default;

  /*======================= Assignments ========================*/
  FirstK& operator=(const FirstK& /*unused*/) = default;

  FirstK& operator=(FirstK&& /*unused*/) = default;

  /*==================== First_k Computation ===================*/
  [[nodiscard]] UsetT<StringT> compute_first_k(const StringT& str) const {
    UsetT<StringT> result = {StringT{}};
    for (const auto& sym : str) {
      result = concat_k_sets(result, first_k_.at(sym));
    }
    return result;
  }

 private:
  /*========================== Impls ===========================*/
  void initialize_first_k_sets(const Grammar& grammar) {
    for (const auto& terminal : grammar.terminals) {
      first_k_[terminal] = {{terminal}};
    }
  }

  void compute_first_k_fixed_point(const Grammar& grammar) {
    while (update_first_k_in_single_iteration(grammar)) {
    }
  }

  bool update_first_k_in_single_iteration(const Grammar& grammar) {
    bool changed = false;
    for (const auto& rule : grammar.rules) {
      auto rhs_first_k = compute_first_k_for_rhs(rule);
      changed |= update_lhs_first_k_if_changed(rule, rhs_first_k);
    }
    return changed;
  }

  [[nodiscard]] UsetT<StringT> compute_first_k_for_rhs(
      const details::Rule& rule) {
    UsetT<StringT> rhs_first_k_result = {StringT{}};

    for (const auto& sym : rule.rhs) {
      auto sym_first_k = first_k_[sym];
      rhs_first_k_result = concat_k_sets(rhs_first_k_result, sym_first_k);
    }

    return rhs_first_k_result;
  }

  bool update_lhs_first_k_if_changed(const details::Rule& rule,
                                     const UsetT<StringT>& rhs_first_k) {
    auto& lhs_first_k = first_k_[rule.lhs];
    const auto kSizeBefore = lhs_first_k.size();
    union_k_sets(lhs_first_k, rhs_first_k);
    const auto kSizeAfter = lhs_first_k.size();
    return kSizeBefore != kSizeAfter;
  }

  [[nodiscard]] UsetT<StringT> concat_k_sets(
      const UsetT<StringT>& lhs_set, const UsetT<StringT>& rhs_set) const {
    UsetT<StringT> result;
    for (const auto& lhs : lhs_set) {
      if (lhs.size() >= k_) {
        result.emplace(lhs.substr(0, k_));
        continue;
      }
      for (const auto& rhs : rhs_set) {
        StringT added = lhs + rhs;
        if (added.size() > k_) {
          added.resize(k_);
        }
        result.emplace(std::move(added));
      }
    }
    return result;
  }

  static void union_k_sets(UsetT<StringT>& lhs_set,
                           const UsetT<StringT>& rhs_set) {
    for (const auto& rhs : rhs_set) {
      lhs_set.emplace(rhs);
    }
  }

  /*======================= Data Fields ========================*/
  std::size_t k_{};
  UmapT<CharT, UsetT<StringT>> first_k_;
};
}  // namespace lrk_parser::details
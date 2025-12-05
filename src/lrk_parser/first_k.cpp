#include "lrk_parser/first_k.hpp"

lrk_parser::UsetT<lrk_parser::StringT>
lrk_parser::details::FirstK::compute_first_k(const StringT& str) const {
  UsetT<StringT> result = {StringT{}};
  for (const auto& sym : str) {
    result = concat_k_sets(result, first_k_.at(sym));
  }
  return result;
}

void lrk_parser::details::FirstK::initialize_first_k_sets(
    const Grammar& grammar) {
  for (const auto& terminal : grammar.get_terminals()) {
    first_k_[terminal] = {{terminal}};
  }
}

void lrk_parser::details::FirstK::compute_first_k_fixed_point(
    const Grammar& grammar) {
  while (update_first_k_in_single_iteration(grammar)) {
  }
}

bool lrk_parser::details::FirstK::update_first_k_in_single_iteration(
    const Grammar& grammar) {
  bool changed = false;
  for (const auto& rule : grammar.get_rules()) {
    auto rhs_first_k = compute_first_k_for_rhs(rule);
    changed |= update_lhs_first_k_if_changed(rule, rhs_first_k);
  }
  return changed;
}

lrk_parser::UsetT<lrk_parser::StringT>
lrk_parser::details::FirstK::compute_first_k_for_rhs(
    const details::Rule& rule) {
  UsetT<StringT> rhs_first_k_result = {StringT{}};

  for (const auto& sym : rule.rhs) {
    auto sym_first_k = first_k_[sym];
    rhs_first_k_result = concat_k_sets(rhs_first_k_result, sym_first_k);
  }

  return rhs_first_k_result;
}

bool lrk_parser::details::FirstK::update_lhs_first_k_if_changed(
    const details::Rule& rule, const UsetT<StringT>& rhs_first_k) {
  auto& lhs_first_k = first_k_[rule.lhs];
  const auto kSizeBefore = lhs_first_k.size();
  union_k_sets(lhs_first_k, rhs_first_k);
  const auto kSizeAfter = lhs_first_k.size();
  return kSizeBefore != kSizeAfter;
}

lrk_parser::UsetT<lrk_parser::StringT>
lrk_parser::details::FirstK::concat_k_sets(
    const UsetT<StringT>& lhs_set, const UsetT<StringT>& rhs_set) const {
  UsetT<StringT> result;
  for (const auto& lhs : lhs_set) {
    if (lhs.size() >= k_) {
      result.emplace(lhs.substr(0, k_));
      continue;
    }

    const std::size_t kNeededFromRhs = k_ - lhs.size();

    for (const auto& rhs : rhs_set) {
      StringT added = lhs + rhs.substr(0, kNeededFromRhs);
      result.emplace(std::move(added));
    }
  }
  return result;
}

void lrk_parser::details::FirstK::union_k_sets(UsetT<StringT>& lhs_set,
                                               const UsetT<StringT>& rhs_set) {
  for (const auto& rhs : rhs_set) {
    lhs_set.emplace(rhs);
  }
}

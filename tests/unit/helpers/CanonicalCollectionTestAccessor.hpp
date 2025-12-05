#pragma once

#include "lrk_parser/canonical_collection.hpp"
#include "lrk_parser/first_k.hpp"
#include "lrk_parser/grammar.hpp"

namespace lrk_parser::details {

class CanonicalCollectionTestAccessor : public CanonicalCollection {
 public:
  CanonicalCollectionTestAccessor(const Grammar& grammar, const FirstK& fk)
      : CanonicalCollection(grammar, fk) {}

  details::Situations PublicClosure(const Grammar& grammar,
                                    const FirstK& fist_k,
                                    details::Situations kernel_set) const {
    return closure(grammar, fist_k, std::move(kernel_set));
  }

  details::Situations PublicComputeGo(const Grammar& grammar,
                                      const FirstK& fist_k, std::size_t I_idx,
                                      CharT X) const {
    return compute_go_situation(grammar, fist_k, I_idx, X);
  }

  std::pair<StateIdT, bool> PublicInsertSituations(details::Situations I) {
    return insert_sutiations(std::move(I));
  }

  const StatesT& GetStatesInternal() const { return states_; }
};

}  // namespace lrk_parser::details
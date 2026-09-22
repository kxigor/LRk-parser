#pragma once

#include "lrk_parser/canonical_collection.hpp"
#include "lrk_parser/first_k.hpp"
#include "lrk_parser/grammar.hpp"

namespace lrk_parser::details {

class CanonicalCollectionTestAccessor : public CanonicalCollection {
 public:
  CanonicalCollectionTestAccessor(const PreparedGrammar& grammar,
                                  const FirstK& fk)
      : CanonicalCollection(grammar, fk) {}

  details::Situations PublicClosure(const PreparedGrammar& grammar,
                                    const FirstK& fist_k,
                                    details::Situations kernel_set) const {
    CanonicalCollection::CCC ctx(grammar, fist_k);
    return closure(ctx, std::move(kernel_set));
  }

  details::Situations PublicComputeGo(const PreparedGrammar& grammar,
                                      const FirstK& fist_k, StateId I_idx,
                                      CharT X) const {
    CanonicalCollection::CCC ctx(grammar, fist_k);
    return compute_go_situation(ctx, I_idx, EncodeSymbol(X));
  }

  std::pair<StateId, bool> PublicInsertSituations(details::Situations I) {
    return insert_sutiations(std::move(I));
  }

  const StatesT& GetStatesInternal() const { return states_; }
};

}  // namespace lrk_parser::details
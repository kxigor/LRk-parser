#pragma once

#include <utility>
#include <vector>

#include "first_k.hpp"
#include "prepared_grammar.hpp"
#include "situation.hpp"
#include "tables_base.hpp"

namespace lrk_parser::details {

class CanonicalCollection {
 public:
  [[nodiscard]] static CanonicalCollection Build(const PreparedGrammar& grammar,
                                                 const FirstK& first);

  const std::vector<Situations>& States() const { return states_; }
  const TransitionMap& Transitions() const { return transitions_; }

  TransitionMap TakeTransitions() && { return std::move(transitions_); }

 private:
  CanonicalCollection() = default;

  std::vector<Situations> states_;
  TransitionMap transitions_;
};

}  // namespace lrk_parser::details

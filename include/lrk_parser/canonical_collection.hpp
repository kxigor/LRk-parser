#pragma once

#include <unordered_map>
#include <utility>
#include <vector>

#include "details/prepared_grammar.hpp"
#include "first_k.hpp"
#include "situation.hpp"
#include "tables_base.hpp"

namespace lrk_parser::details {

class CanonicalCollection {
 public:
  using TransitionMap =
      std::unordered_map<TransitionKey, StateId, TransitionKeyHash>;

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

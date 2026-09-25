#pragma once

#include <cstddef>
#include <expected>
#include <iosfwd>
#include <unordered_map>

#include "canonical_collection.hpp"
#include "first_k.hpp"
#include "prepared_grammar.hpp"
#include "situation.hpp"
#include "tables_base.hpp"

namespace lrk_parser::details {

struct ActionSource {
  Action action;
  Situation situation;
  PreparedRule rule;
};

struct ActionConflict {
  std::size_t k;
  StateId state;
  StringT lookahead;
  ActionSource existing;
  ActionSource incoming;
};

class ActionTable {
 public:
  ActionTable() = default;

  [[nodiscard]] static std::expected<ActionTable, ActionConflict> Build(
      const PreparedGrammar& grammar, const FirstK& first,
      const CanonicalCollection& collection);

  friend std::ostream& operator<<(std::ostream& os, const ActionTable& table);

  [[nodiscard]] const Action* FindAction(const ActionKey& key) const;

 private:
  class Builder;

  std::unordered_map<ActionKey, Action, ActionKeyHash> action_table_;
};

}  // namespace lrk_parser::details

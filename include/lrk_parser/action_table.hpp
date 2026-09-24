#pragma once

#include <cstddef>
#include <expected>
#include <format>
#include <ostream>

#include "canonical_collection.hpp"
#include "details/prepared_grammar.hpp"
#include "first_k.hpp"
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

  friend struct std::formatter<ActionTable>;
  friend std::ostream& operator<<(std::ostream& os, const ActionTable& table);

  [[nodiscard]] bool HasParseAction(const ActionKey& key) const;
  [[nodiscard]] const Action& GetParseAction(const ActionKey& key) const;

 private:
  UmapT<ActionKey, Action, ActionKeyHash> action_table_;
};

}  // namespace lrk_parser::details

#pragma once

#include <cstddef>
#include <expected>
#include <iosfwd>
#include <variant>
#include <vector>

#include "action_table.hpp"
#include "config.hpp"
#include "details/prepared_grammar.hpp"
#include "goto_table.hpp"
#include "grammar.hpp"

namespace lrk_parser {

struct InvalidLookahead {
  std::size_t k;
};

using CompileError = std::variant<InvalidLookahead, details::ActionConflict>;

class Parser {
 public:
  [[nodiscard]] static std::expected<Parser, CompileError> Compile(
      const Grammar& grammar, std::size_t k);

  friend std::ostream& operator<<(std::ostream& os, const Parser& parser);

  [[nodiscard]] bool Accepts(StringViewT word) const;
  [[nodiscard]] std::size_t Lookahead() const { return k_; }

 private:
  Parser(std::size_t k, details::PreparedGrammar grammar,
         details::ActionTable actions, details::GotoTable gotos);
  [[nodiscard]] bool ApplyReduction(std::vector<details::StateId>& stack,
                                    details::RuleId rule) const;

  std::size_t k_;
  details::PreparedGrammar grammar_;
  details::GotoTable goto_table_;
  details::ActionTable action_table_;
};

}  // namespace lrk_parser

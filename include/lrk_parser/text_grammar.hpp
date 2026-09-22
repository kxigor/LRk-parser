#pragma once

#include <cstddef>
#include <expected>
#include <variant>
#include <vector>

#include "config.hpp"
#include "grammar.hpp"

namespace lrk_parser {

struct RuleSyntaxError {
  std::size_t rule_index;
};

using TextGrammarError = std::variant<RuleSyntaxError, GrammarError>;

[[nodiscard]] std::expected<Grammar, TextGrammarError> ParseGrammar(
    StringT terminals, StringT nonterminals, std::vector<StringT> rules,
    CharT start);

}  // namespace lrk_parser

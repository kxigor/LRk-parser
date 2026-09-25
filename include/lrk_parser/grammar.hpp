#pragma once

#include <cstddef>
#include <expected>
#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <vector>

#include "config.hpp"
#include "rule.hpp"

namespace lrk_parser {

struct GrammarSpec {
  StringT terminals;
  StringT nonterminals;
  std::vector<Rule> rules;
  CharT start{};
};

enum class GrammarErrorKind {
  OverlappingAlphabets,
  InvalidStart,
  InvalidLhs,
  UnknownRhsSymbol,
  MissingProduction,
};

struct GrammarError {
  GrammarErrorKind kind;
  CharT symbol;
  std::optional<std::size_t> rule_index;
};

struct RuleSyntaxError {
  std::size_t rule_index;
};

using TextGrammarError = std::variant<RuleSyntaxError, GrammarError>;

class Grammar {
 public:
  [[nodiscard]] static std::expected<Grammar, GrammarError> FromSpec(
      GrammarSpec spec);
  [[nodiscard]] static std::expected<Grammar, TextGrammarError> FromTextRules(
      StringT terminals, StringT nonterminals, std::vector<StringT> rules,
      CharT start);

  StringViewT Terminals() const { return spec_.terminals; }
  StringViewT Nonterminals() const { return spec_.nonterminals; }
  std::span<const Rule> Rules() const { return spec_.rules; }
  CharT Start() const { return spec_.start; }

 private:
  explicit Grammar(GrammarSpec spec) : spec_{std::move(spec)} {}

  GrammarSpec spec_;
};

}  // namespace lrk_parser

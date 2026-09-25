#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <variant>
#include <vector>

#include "lrk_parser/grammar.hpp"

namespace lrk_parser {
namespace {

TEST(TextGrammar, ParsesRulesAndRemovesAsciiWhitespace) {
  const auto result = Grammar::FromTextRules(
      "ab", "SA", {" \tS \n-\r> A a\v", "A -> b\f", " A -> "}, 'S');

  ASSERT_TRUE(result.has_value());
  const std::vector<Rule> expected{{'S', "Aa"}, {'A', "b"}, {'A', ""}};
  EXPECT_TRUE(std::ranges::equal(result->Rules(), expected));
  EXPECT_EQ(result->Start(), 'S');
}

TEST(TextGrammar, ReportsMalformedRuleIndex) {
  for (const StringT invalid : {"", "S", "S-", "S>a", "AS->a", "S=>a"}) {
    SCOPED_TRACE(invalid);
    const auto result =
        Grammar::FromTextRules("a", "S", {"S->a", invalid}, 'S');

    ASSERT_FALSE(result.has_value());
    const auto* error = std::get_if<RuleSyntaxError>(&result.error());
    ASSERT_NE(error, nullptr);
    EXPECT_EQ(error->rule_index, 1);
  }
}

TEST(TextGrammar, PreservesValidationErrorAndRuleIndex) {
  const auto result =
      Grammar::FromTextRules("a", "S", {" S -> a ", "S -> X"}, 'S');

  ASSERT_FALSE(result.has_value());
  const auto* error = std::get_if<GrammarError>(&result.error());
  ASSERT_NE(error, nullptr);
  EXPECT_EQ(error->kind, GrammarErrorKind::UnknownRhsSymbol);
  EXPECT_EQ(error->symbol, 'X');
  EXPECT_EQ(error->rule_index, 1);
}

TEST(TextGrammar, ValidatesStartAndMissingProductions) {
  for (const auto& result :
       {Grammar::FromTextRules("a", "S", {"S->a"}, 'X'),
        Grammar::FromTextRules("a", "S", {}, 'S'),
        Grammar::FromTextRules("a", "SA", {"S->A"}, 'S')}) {
    ASSERT_FALSE(result.has_value());
    EXPECT_TRUE(std::holds_alternative<GrammarError>(result.error()));
  }
}

TEST(TextGrammar, PreservesDuplicateRules) {
  const auto result = Grammar::FromTextRules("a", "S", {"S->a", "S -> a"}, 'S');

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->Rules().size(), 2);
  EXPECT_EQ(result->Rules()[0], result->Rules()[1]);
}

TEST(TextGrammar, AllowsArrowCharactersAsSymbols) {
  const auto result = Grammar::FromTextRules("->", "S", {"S->->"}, 'S');
  const auto lhs = Grammar::FromTextRules("a", "-", {"-->a"}, '-');

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->Rules()[0].rhs, "->");
  ASSERT_TRUE(lhs.has_value());
  EXPECT_EQ(lhs->Rules()[0].lhs, '-');
}

TEST(TextGrammar, PreservesNullAndHighBitBytes) {
  const StringT bytes{'\0', static_cast<CharT>(0xFF)};
  const auto result = Grammar::FromTextRules(bytes, "@", {"@->" + bytes}, '@');

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->Rules()[0], (Rule{'@', bytes}));
}

}  // namespace
}  // namespace lrk_parser

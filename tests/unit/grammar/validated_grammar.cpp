#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "lrk_parser/grammar.hpp"

namespace lrk_parser {
namespace {

TEST(ValidatedGrammar, PreservesRulesWithoutAugmentationOrNormalization) {
  const GrammarSpec spec{
      "a \t", "SA", {{'S', " A\t"}, {'A', "a"}, {'A', ""}, {'A', "a"}}, 'S'};
  const auto result = MakeGrammar(spec);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->Terminals(), spec.terminals);
  EXPECT_EQ(result->Nonterminals(), spec.nonterminals);
  EXPECT_EQ(result->Start(), spec.start);
  EXPECT_TRUE(std::ranges::equal(result->Rules(), spec.rules));
}

TEST(ValidatedGrammar, OwnsInputAfterMutationAndDestruction) {
  const auto result = [] {
    GrammarSpec spec{"a", "S", {{'S', "a"}}, 'S'};
    auto grammar = MakeGrammar(spec);
    spec.terminals = "b";
    spec.nonterminals = "B";
    spec.rules[0] = {'B', "b"};
    spec.start = 'B';
    return grammar;
  }();

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->Terminals(), "a");
  EXPECT_EQ(result->Nonterminals(), "S");
  EXPECT_EQ(result->Start(), 'S');
  ASSERT_EQ(result->Rules().size(), 1);
  EXPECT_EQ(result->Rules()[0], (Rule{'S', "a"}));
}

TEST(ValidatedGrammar, AllowsEmptyTerminalAlphabetAndEpsilon) {
  const auto result = MakeGrammar({"", "S", {{'S', ""}}, 'S'});

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->Rules().size(), 1);
  EXPECT_TRUE(result->Rules()[0].rhs.empty());
}

TEST(ValidatedGrammar, AllowsUnusedNonterminalWithoutProduction) {
  EXPECT_TRUE(MakeGrammar({"a", "SA", {{'S', "a"}}, 'S'}).has_value());
}

TEST(ValidatedGrammar, PreservesUnreachableAndNonproductiveRules) {
  const GrammarSpec spec{
      "ab", "SAB", {{'S', "aS"}, {'A', "A"}, {'B', "b"}, {'B', "b"}}, 'S'};
  const auto result = MakeGrammar(spec);

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(std::ranges::equal(result->Rules(), spec.rules));
}

TEST(ValidatedGrammar, AllowsAtSignInEitherAlphabet) {
  EXPECT_TRUE(MakeGrammar({"@", "S", {{'S', "@"}}, 'S'}).has_value());
  EXPECT_TRUE(MakeGrammar({"a", "@", {{'@', "a"}}, '@'}).has_value());
}

TEST(ValidatedGrammar, AllowsNullAndHighBitBytesInEitherAlphabet) {
  const StringT bytes{'\0', static_cast<CharT>(0xFF)};
  const auto terminals = MakeGrammar({bytes, "S", {{'S', bytes}}, 'S'});

  ASSERT_TRUE(terminals.has_value());
  EXPECT_EQ(terminals->Terminals(), bytes);
  EXPECT_EQ(terminals->Rules()[0].rhs, bytes);

  const auto nonterminals = MakeGrammar(
      {"a", bytes, {{'\0', bytes.substr(1)}, {bytes[1], "a"}}, '\0'});

  ASSERT_TRUE(nonterminals.has_value());
  EXPECT_EQ(nonterminals->Start(), '\0');
  EXPECT_EQ(nonterminals->Nonterminals(), bytes);
}

TEST(ValidatedGrammar, RejectsOverlappingAlphabets) {
  const auto result = MakeGrammar({"aS", "S", {{'S', "a"}}, 'S'});

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().kind, GrammarErrorKind::OverlappingAlphabets);
  EXPECT_EQ(result.error().symbol, 'S');
  EXPECT_FALSE(result.error().rule_index.has_value());
}

TEST(ValidatedGrammar, RejectsTerminalOrUndeclaredStart) {
  for (CharT start : {'a', 'X'}) {
    const auto result = MakeGrammar({"a", "S", {{'S', "a"}}, start});

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().kind, GrammarErrorKind::InvalidStart);
    EXPECT_EQ(result.error().symbol, start);
    EXPECT_FALSE(result.error().rule_index.has_value());
  }
}

TEST(ValidatedGrammar, RejectsEmptyNonterminalAlphabet) {
  const auto result = MakeGrammar({"", "", {}, 'S'});

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().kind, GrammarErrorKind::InvalidStart);
}

TEST(ValidatedGrammar, RejectsStartWithoutProduction) {
  for (const auto& rules :
       {std::vector<Rule>{}, std::vector<Rule>{{'A', "a"}}}) {
    const auto result = MakeGrammar({"a", "SA", rules, 'S'});

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().kind, GrammarErrorKind::MissingProduction);
    EXPECT_EQ(result.error().symbol, 'S');
    EXPECT_FALSE(result.error().rule_index.has_value());
  }
}

TEST(ValidatedGrammar, RejectsTerminalOrUndeclaredLhs) {
  for (CharT lhs : {'a', 'X'}) {
    const auto result = MakeGrammar({"a", "S", {{'S', "a"}, {lhs, "a"}}, 'S'});

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().kind, GrammarErrorKind::InvalidLhs);
    EXPECT_EQ(result.error().symbol, lhs);
    EXPECT_EQ(result.error().rule_index, 1);
  }
}

TEST(ValidatedGrammar, RejectsUnknownRhsIncludingUndeclaredWhitespace) {
  for (CharT symbol : {'X', ' ', '\0', static_cast<CharT>(0xFF)}) {
    const auto result =
        MakeGrammar({"a", "S", {{'S', "a"}, {'S', StringT{'a', symbol}}}, 'S'});

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().kind, GrammarErrorKind::UnknownRhsSymbol);
    EXPECT_EQ(result.error().symbol, symbol);
    EXPECT_EQ(result.error().rule_index, 1);
  }
}

TEST(ValidatedGrammar, RejectsReferencedNonterminalWithoutProduction) {
  const auto result = MakeGrammar({"a", "SA", {{'S', "a"}, {'S', "aA"}}, 'S'});

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().kind, GrammarErrorKind::MissingProduction);
  EXPECT_EQ(result.error().symbol, 'A');
  EXPECT_EQ(result.error().rule_index, 1);
}

TEST(ValidatedGrammar, ChecksReferencesInUnreachableRules) {
  const auto result = MakeGrammar({"a", "SAB", {{'S', "a"}, {'B', "A"}}, 'S'});

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().kind, GrammarErrorKind::MissingProduction);
  EXPECT_EQ(result.error().symbol, 'A');
  EXPECT_EQ(result.error().rule_index, 1);
}

}  // namespace
}  // namespace lrk_parser

#include "lrk_parser/details/prepared_grammar.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

namespace lrk_parser::details {
namespace {

TEST(PreparedGrammar, AugmentsWithoutChangingUserGrammar) {
  const auto grammar = MakeGrammar({"a", "@A", {{'@', "A"}, {'A', "a"}}, '@'});
  ASSERT_TRUE(grammar.has_value());
  const PreparedGrammar prepared(*grammar);

  ASSERT_EQ(grammar->Rules().size(), 2);
  EXPECT_EQ(grammar->Rules()[0], (Rule{'@', "A"}));
  ASSERT_EQ(prepared.Rules().size(), 3);
  EXPECT_EQ(prepared.GetRule(kStartRule).lhs, kAugmentedStart);
  EXPECT_EQ(prepared.GetRule(kStartRule).rhs, EncodeSymbols("@"));
  EXPECT_NE(kAugmentedStart, EncodeSymbol('@'));
  EXPECT_TRUE(prepared.IsNonterminal(kAugmentedStart));
  EXPECT_FALSE(prepared.IsTerminal(kAugmentedStart));
}

TEST(PreparedGrammar, IndexesDuplicateRulesSeparatelyInInputOrder) {
  const auto grammar = MakeGrammar(
      {"ab", "SAB", {{'S', "A"}, {'A', "a"}, {'S', "b"}, {'A', "a"}}, 'S'});
  ASSERT_TRUE(grammar.has_value());
  const PreparedGrammar prepared(*grammar);

  const std::vector<RuleId> s_rules{RuleId{1}, RuleId{3}};
  const std::vector<RuleId> a_rules{RuleId{2}, RuleId{4}};
  EXPECT_TRUE(
      std::ranges::equal(prepared.RulesFor(EncodeSymbol('S')), s_rules));
  EXPECT_TRUE(
      std::ranges::equal(prepared.RulesFor(EncodeSymbol('A')), a_rules));
  EXPECT_TRUE(prepared.RulesFor(EncodeSymbol('B')).empty());
  EXPECT_TRUE(prepared.RulesFor(EncodeSymbol('X')).empty());
  EXPECT_EQ(prepared.GetRule(RuleId{2}).rhs, EncodeSymbols("a"));
  EXPECT_EQ(prepared.GetRule(RuleId{4}).rhs, EncodeSymbols("a"));
}

TEST(PreparedGrammar, OwnsRulesAfterSourceDestruction) {
  const auto prepared = [] {
    const auto grammar = MakeGrammar({"a", "S", {{'S', "aS"}, {'S', ""}}, 'S'});
    return PreparedGrammar(grammar.value());
  }();

  ASSERT_EQ(prepared.Rules().size(), 3);
  EXPECT_EQ(prepared.GetRule(RuleId{1}).rhs, EncodeSymbols("aS"));
  EXPECT_TRUE(prepared.GetRule(RuleId{2}).rhs.empty());
  EXPECT_EQ(prepared.RulesFor(EncodeSymbol('S')).size(), 2);
}

TEST(PreparedGrammar, ByteIdsDoNotDependOnAlphabetOrderOrDuplicates) {
  const auto first =
      MakeGrammar({"baa", "ASS", {{'S', "bAa"}, {'A', ""}}, 'S'});
  const auto second = MakeGrammar({"ab", "SA", {{'S', "bAa"}, {'A', ""}}, 'S'});
  ASSERT_TRUE(first.has_value());
  ASSERT_TRUE(second.has_value());
  const PreparedGrammar a(*first);
  const PreparedGrammar b(*second);

  EXPECT_EQ(a.Terminals().size(), 2);
  EXPECT_EQ(a.Nonterminals().size(), 3);
  EXPECT_EQ(a.GetRule(RuleId{1}).rhs, b.GetRule(RuleId{1}).rhs);
  EXPECT_EQ(a.GetRule(RuleId{1}).lhs, b.GetRule(RuleId{1}).lhs);
}

TEST(PreparedGrammar, EveryByteHasDistinctIdAndRoundTrips) {
  std::unordered_set<SymbolId> ids;
  for (std::size_t value = 0; value < std::to_underlying(kAugmentedStart);
       ++value) {
    const auto byte = static_cast<CharT>(value);
    const auto id = EncodeSymbol(byte);
    EXPECT_NE(id, kAugmentedStart);
    EXPECT_TRUE(ids.insert(id).second);
    EXPECT_EQ(DecodeSymbol(id), byte);
  }
}

}  // namespace
}  // namespace lrk_parser::details

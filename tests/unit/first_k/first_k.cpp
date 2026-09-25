#include "lrk_parser/details/first_k.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <sstream>
#include <utility>

#include "lrk_parser/output.hpp"

namespace lrk_parser::details {
namespace {

const PrefixSet& ForSymbol(const FirstK& first, CharT symbol) {
  return first.Sets().at(EncodeSymbol(symbol));
}

TEST(FirstK, NullableAlternativesAtK1) {
  const PreparedGrammar grammar{Grammar::FromSpec({"abcd",
                                                   "SAB",
                                                   {{'S', "AB"},
                                                    {'S', "c"},
                                                    {'A', "a"},
                                                    {'A', ""},
                                                    {'B', "b"},
                                                    {'B', ""}},
                                                   'S'})
                                    .value()};
  const auto first = FirstK::Compute(grammar, 1);

  EXPECT_EQ(first.Lookahead(), 1);
  for (CharT symbol : StringT{"abcd"}) {
    EXPECT_EQ(ForSymbol(first, symbol), PrefixSet{StringT{symbol}});
  }
  EXPECT_EQ(ForSymbol(first, 'A'), (PrefixSet{"a", ""}));
  EXPECT_EQ(ForSymbol(first, 'B'), (PrefixSet{"b", ""}));
  EXPECT_EQ(ForSymbol(first, 'S'), (PrefixSet{"a", "b", "c", ""}));
  EXPECT_EQ(first.Sets().at(kAugmentedStart), ForSymbol(first, 'S'));
}

TEST(FirstK, NullableChainAtK2) {
  const PreparedGrammar grammar{
      Grammar::FromSpec({"abcd",
                         "SAB",
                         {{'S', "Aa"}, {'A', "Bc"}, {'B', "d"}, {'B', ""}},
                         'S'})
          .value()};
  const auto first = FirstK::Compute(grammar, 2);

  EXPECT_EQ(ForSymbol(first, 'B'), (PrefixSet{"d", ""}));
  EXPECT_EQ(ForSymbol(first, 'A'), (PrefixSet{"dc", "c"}));
  EXPECT_EQ(ForSymbol(first, 'S'), (PrefixSet{"dc", "ca"}));
}

TEST(FirstK, SequenceAtK3) {
  const PreparedGrammar grammar{
      Grammar::FromSpec(
          {"ac",
           "SAB",
           {{'S', "ABa"}, {'A', "a"}, {'A', "Bc"}, {'B', "c"}, {'B', ""}},
           'S'})
          .value()};
  const auto first = FirstK::Compute(grammar, 3);

  EXPECT_EQ(first.ForSequence(EncodeSymbols("ABa")),
            (PrefixSet{"aa", "aca", "ccc", "ca", "cca"}));
}

TEST(FirstK, FullSetsAtK3) {
  const PreparedGrammar grammar{Grammar::FromSpec({"abcde",
                                                   "SABCD",
                                                   {{'S', "ABC"},
                                                    {'A', "a"},
                                                    {'A', "BD"},
                                                    {'B', "b"},
                                                    {'B', ""},
                                                    {'C', "c"},
                                                    {'C', "De"},
                                                    {'D', "d"},
                                                    {'D', ""}},
                                                   'S'})
                                    .value()};
  const auto first = FirstK::Compute(grammar, 3);

  EXPECT_EQ(ForSymbol(first, 'A'), (PrefixSet{"a", "", "d", "b", "bd"}));
  EXPECT_EQ(ForSymbol(first, 'B'), (PrefixSet{"b", ""}));
  EXPECT_EQ(ForSymbol(first, 'C'), (PrefixSet{"c", "e", "de"}));
  EXPECT_EQ(ForSymbol(first, 'D'), (PrefixSet{"d", ""}));
  EXPECT_EQ(ForSymbol(first, 'S'),
            (PrefixSet{"ac",  "ae",  "ade", "abc", "abe", "abd", "c",   "e",
                       "de",  "bc",  "be",  "bde", "bbc", "bbe", "bbd", "dc",
                       "dde", "dbc", "dbe", "dbd", "bdc", "bdd", "bdb"}));
}

TEST(FirstK, EmptySequenceAndShortLookahead) {
  const PreparedGrammar grammar{
      Grammar::FromSpec({"abc", "S", {{'S', "a"}, {'S', ""}}, 'S'}).value()};
  const auto first = FirstK::Compute(grammar, 3);

  EXPECT_EQ(first.ForSequence({}), PrefixSet{""});
  EXPECT_EQ(first.ForSequence({}, "b"), PrefixSet{"b"});
  EXPECT_EQ(first.ForSequence(EncodeSymbols("S"), "b"), (PrefixSet{"ab", "b"}));
  EXPECT_EQ(first.ForSequence(EncodeSymbols("S"), "bcbc"),
            (PrefixSet{"abc", "bcb"}));
}

TEST(FirstK, EpsilonNonterminalPreservesSequencePrefixes) {
  const PreparedGrammar grammar{
      Grammar::FromSpec(
          {"ab",
           "SAE",
           {{'S', "AE"}, {'A', ""}, {'A', "a"}, {'A', "ab"}, {'E', ""}},
           'S'})
          .value()};
  const auto first = FirstK::Compute(grammar, 3);
  const PrefixSet expected{"", "a", "ab"};

  EXPECT_EQ(first.ForSequence(EncodeSymbols("AE")), expected);
  EXPECT_EQ(first.ForSequence(EncodeSymbols("EA")), expected);
}

TEST(FirstK, NonproductiveLeadingNonterminalProducesNoPrefixes) {
  const PreparedGrammar grammar{
      Grammar::FromSpec(
          {"a", "SAB", {{'S', "AB"}, {'A', "A"}, {'B', "a"}, {'B', ""}}, 'S'})
          .value()};
  const auto first = FirstK::Compute(grammar, 2);

  EXPECT_TRUE(first.ForSequence(EncodeSymbols("A")).empty());
  EXPECT_TRUE(first.ForSequence(EncodeSymbols("AB")).empty());
}

TEST(FirstK, TruncatesAndDeduplicatesSequencePrefixes) {
  const PreparedGrammar grammar{
      Grammar::FromSpec(
          {"abcd",
           "SAB",
           {{'S', "AB"}, {'A', "a"}, {'A', "ab"}, {'B', "bc"}, {'B', "bd"}},
           'S'})
          .value()};
  const auto first = FirstK::Compute(grammar, 2);
  const auto longer = FirstK::Compute(grammar, 3);

  EXPECT_EQ(first.ForSequence(EncodeSymbols("AB")), PrefixSet{"ab"});
  EXPECT_EQ(longer.ForSequence(EncodeSymbols("AB")),
            (PrefixSet{"abc", "abd", "abb"}));
  EXPECT_EQ(first.ForSequence(EncodeSymbols("A")), (PrefixSet{"a", "ab"}));
  EXPECT_EQ(first.ForSequence(EncodeSymbols("B")), (PrefixSet{"bc", "bd"}));
}

TEST(FirstK, NullableRecursionConverges) {
  const PreparedGrammar grammar{
      Grammar::FromSpec(
          {"a", "SAB", {{'S', "A"}, {'A', "B"}, {'B', "aA"}, {'B', ""}}, 'S'})
          .value()};
  for (std::size_t k : {1U, 2U, 3U}) {
    const auto first = FirstK::Compute(grammar, k);
    PrefixSet expected;
    for (std::size_t length = 0; length <= k; ++length) {
      expected.insert(StringT(length, 'a'));
    }
    for (CharT symbol : StringT{"SAB"}) {
      EXPECT_EQ(ForSymbol(first, symbol), expected);
    }
  }
}

TEST(FirstK, RuleAndAlphabetOrderDoNotChangeSets) {
  GrammarSpec spec{
      "ab", "SA", {{'S', "Aa"}, {'S', "b"}, {'A', "bA"}, {'A', ""}}, 'S'};
  std::array<std::size_t, 4> order{0, 1, 2, 3};
  const auto rules = spec.rules;
  const auto reference =
      FirstK::Compute(PreparedGrammar{Grammar::FromSpec(spec).value()}, 3);
  std::ranges::reverse(spec.terminals);
  std::ranges::reverse(spec.nonterminals);

  do {
    for (std::size_t i = 0; i < order.size(); ++i) {
      spec.rules[i] = rules[order[i]];
    }
    const auto first =
        FirstK::Compute(PreparedGrammar{Grammar::FromSpec(spec).value()}, 3);
    EXPECT_EQ(first.Sets(), reference.Sets());
  } while (std::next_permutation(order.begin(), order.end()));
}

TEST(FirstK, ResultsOwnTheirDataAndDoNotMixGrammarsOrLookaheads) {
  const auto first = [] {
    const PreparedGrammar grammar{
        Grammar::FromSpec({"a", "S", {{'S', "aS"}, {'S', ""}}, 'S'}).value()};
    return FirstK::Compute(grammar, 2);
  }();
  const auto other = FirstK::Compute(
      PreparedGrammar{
          Grammar::FromSpec({"b", "S", {{'S', "bbb"}}, 'S'}).value()},
      3);

  EXPECT_EQ(first.Lookahead(), 2);
  EXPECT_EQ(ForSymbol(first, 'S'), (PrefixSet{"", "a", "aa"}));
  EXPECT_EQ(other.Lookahead(), 3);
  EXPECT_EQ(ForSymbol(other, 'S'), PrefixSet{"bbb"});
  EXPECT_FALSE(first.Sets().contains(EncodeSymbol('b')));
  EXPECT_FALSE(other.Sets().contains(EncodeSymbol('a')));

  std::ostringstream output;
  EXPECT_NO_THROW(output << first);
  EXPECT_NE(output.str().find("aa"), std::string::npos);
}

TEST(FirstK, HandlesNullHighBitAndWhitespaceTerminals) {
  const StringT word{'\0', static_cast<CharT>(0xFF), ' '};
  const auto first = FirstK::Compute(
      PreparedGrammar{
          Grammar::FromSpec({word, "@", {{'@', word}}, '@'}).value()},
      3);

  EXPECT_EQ(ForSymbol(first, '@'), PrefixSet{word});
  EXPECT_EQ(first.ForSequence({}, word), PrefixSet{word});
}

}  // namespace
}  // namespace lrk_parser::details

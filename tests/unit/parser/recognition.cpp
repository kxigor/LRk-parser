#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "lrk_parser/grammar.hpp"
#include "lrk_parser/parser.hpp"

namespace {

using lrk_parser::CharT;
using lrk_parser::Grammar;
using lrk_parser::Parser;
using lrk_parser::StringT;
using lrk_parser::StringViewT;

template <typename Predicate>
void ExpectLanguage(const Parser& parser, StringViewT alphabet,
                    std::size_t max_length, Predicate accepts) {
  std::vector<StringT> words{StringT{}};
  for (std::size_t length = 0; length <= max_length; ++length) {
    std::vector<StringT> next_words;
    for (const auto& word : words) {
      EXPECT_EQ(parser.Accepts(word), accepts(word))
          << "word: '" << word << "'";
      if (length < max_length) {
        for (CharT symbol : alphabet) {
          next_words.push_back(word + symbol);
        }
      }
    }
    words = std::move(next_words);
  }
}

bool IsBalanced(StringViewT word) {
  std::size_t depth = 0;
  for (CharT symbol : word) {
    if (symbol == '(') {
      ++depth;
    } else if (depth == 0) {
      return false;
    } else {
      --depth;
    }
  }
  return depth == 0;
}

TEST(RecognitionTest, EpsilonLanguage) {
  const auto grammar = Grammar::FromTextRules("a", "S", {"S->"}, 'S').value();
  for (std::size_t k : {1U, 2U, 3U}) {
    SCOPED_TRACE(k);
    auto parser = Parser::Compile(grammar, k);
    ASSERT_TRUE(parser.has_value());
    ExpectLanguage(*parser, "a", 4,
                   [](const StringT& word) { return word.empty(); });
  }
}

TEST(RecognitionTest, EqualNumbersOfAsAndBs) {
  const auto grammar =
      Grammar::FromTextRules("ab", "S", {"S->aSb", "S->"}, 'S').value();
  for (std::size_t k : {1U, 2U, 3U}) {
    SCOPED_TRACE(k);
    auto parser = Parser::Compile(grammar, k);
    ASSERT_TRUE(parser.has_value());
    ExpectLanguage(*parser, "ab", 6, [](const StringT& word) {
      const auto half = word.size() / 2;
      return word == StringT(half, 'a') + StringT(half, 'b');
    });
  }
}

TEST(RecognitionTest, BalancedParentheses) {
  const auto grammar =
      Grammar::FromTextRules("()", "S", {"S->(S)S", "S->"}, 'S').value();
  for (std::size_t k : {1U, 2U, 3U}) {
    SCOPED_TRACE(k);
    auto parser = Parser::Compile(grammar, k);
    ASSERT_TRUE(parser.has_value());
    ExpectLanguage(*parser, "()", 6, IsBalanced);
  }
}

TEST(RecognitionTest, LeftAndRightRecursion) {
  for (const auto* rule : {"S->Sa", "S->aS"}) {
    SCOPED_TRACE(rule);
    const auto grammar =
        Grammar::FromTextRules("ab", "S", {rule, "S->"}, 'S').value();
    auto parser = Parser::Compile(grammar, 1);
    ASSERT_TRUE(parser.has_value());
    ExpectLanguage(*parser, "ab", 6, [](const StringT& word) {
      return word.find('b') == StringT::npos;
    });
  }
}

TEST(RecognitionTest, NullableChain) {
  const auto grammar =
      Grammar::FromTextRules(
          "abc", "SABC",
          {"S->ABC", "A->a", "A->", "B->b", "B->", "C->c", "C->"}, 'S')
          .value();
  const std::vector<StringT> language{"",   "a",  "b",  "c",
                                      "ab", "ac", "bc", "abc"};
  for (std::size_t k : {1U, 2U, 3U}) {
    SCOPED_TRACE(k);
    auto parser = Parser::Compile(grammar, k);
    ASSERT_TRUE(parser.has_value());
    ExpectLanguage(*parser, "abc", 4, [&](const StringT& word) {
      return std::ranges::find(language, word) != language.end();
    });
  }
}

TEST(RecognitionTest, ExplicitLookaheadForLR2AndLR3Grammars) {
  for (std::size_t k : {2U, 3U}) {
    SCOPED_TRACE(k);
    const auto grammar =
        Grammar::FromTextRules("a", "SAB",
                               {"S->A" + StringT(k, 'a'),
                                "S->B" + StringT(k - 1, 'a'), "A->a", "B->a"},
                               'S')
            .value();
    for (std::size_t smaller_k = 1; smaller_k < k; ++smaller_k) {
      EXPECT_FALSE(Parser::Compile(grammar, smaller_k).has_value());
    }
    auto parser = Parser::Compile(grammar, k);
    ASSERT_TRUE(parser.has_value());
    ExpectLanguage(*parser, "ab", 5, [k](const StringT& word) {
      return word == StringT(k, 'a') || word == StringT(k + 1, 'a');
    });
  }
}

TEST(RecognitionTest, NonterminatingRecursionHasEmptyLanguage) {
  const auto grammar = Grammar::FromTextRules("a", "S", {"S->aS"}, 'S').value();
  for (std::size_t k : {1U, 2U, 3U}) {
    SCOPED_TRACE(k);
    auto parser = Parser::Compile(grammar, k);
    ASSERT_TRUE(parser.has_value());
    ExpectLanguage(*parser, "a", 5, [](const StringT&) { return false; });
  }
}

TEST(RecognitionTest, ProductiveBranchBesideNonterminatingRecursion) {
  const auto grammar =
      Grammar::FromTextRules("ab", "SB", {"S->a", "S->B", "B->bB"}, 'S')
          .value();
  for (std::size_t k : {1U, 2U, 3U}) {
    SCOPED_TRACE(k);
    auto parser = Parser::Compile(grammar, k);
    ASSERT_TRUE(parser.has_value());
    ExpectLanguage(*parser, "ab", 5,
                   [](const StringT& word) { return word == "a"; });
  }
}

TEST(RecognitionTest, UnreachableConflictsDoNotAffectStartLanguage) {
  const std::vector<std::vector<StringT>> grammars{
      {"S->a", "A->b", "A->B", "B->b"}, {"S->a", "A->AA", "A->b"}};
  for (const auto& rules : grammars) {
    SCOPED_TRACE(::testing::PrintToString(rules));
    const auto grammar =
        Grammar::FromTextRules("ab", "SAB", rules, 'S').value();
    for (std::size_t k : {1U, 2U, 3U}) {
      SCOPED_TRACE(k);
      auto parser = Parser::Compile(grammar, k);
      ASSERT_TRUE(parser.has_value());
      ExpectLanguage(*parser, "ab", 5,
                     [](const StringT& word) { return word == "a"; });
    }
  }
}

TEST(RecognitionTest, UnitCycleReportsConflictAtFixedLookahead) {
  const auto grammar = Grammar::FromTextRules("a", "S", {"S->S"}, 'S').value();
  for (std::size_t k : {1U, 2U, 3U}) {
    SCOPED_TRACE(k);
    EXPECT_FALSE(Parser::Compile(grammar, k).has_value());
  }
}

TEST(RecognitionTest, CanonicalLR1StatesWithTheSameCoreStaySeparate) {
  const auto grammar = Grammar::FromSpec({"abcde",
                                          "SAB",
                                          {{'S', "aAd"},
                                           {'S', "bAe"},
                                           {'S', "aBe"},
                                           {'S', "bBd"},
                                           {'A', "c"},
                                           {'B', "c"}},
                                          'S'})
                           .value();
  auto parser = Parser::Compile(grammar, 1);
  ASSERT_TRUE(parser.has_value());

  ExpectLanguage(*parser, "abcde", 4, [](StringViewT word) {
    return word == "acd" || word == "ace" || word == "bcd" || word == "bce";
  });
}

TEST(RecognitionTest, AtSignIsAnOrdinaryTerminal) {
  const auto grammar =
      Grammar::FromTextRules("@", "S", {"S->@S", "S->"}, 'S').value();
  for (std::size_t k : {1U, 2U, 3U}) {
    auto parser = Parser::Compile(grammar, k);
    ASSERT_TRUE(parser.has_value());
    ExpectLanguage(*parser, "@a", 4, [](StringViewT word) {
      return word.find_first_not_of('@') == StringViewT::npos;
    });
  }
}

TEST(RecognitionTest, AtSignNonterminalReducesBeforeAccept) {
  const auto grammar =
      Grammar::FromTextRules("ab", "S@", {"S->@b", "@->a@", "@->"}, 'S')
          .value();
  for (std::size_t k : {1U, 2U, 3U}) {
    auto parser = Parser::Compile(grammar, k);
    ASSERT_TRUE(parser.has_value());
    ExpectLanguage(*parser, "ab", 4, [](StringViewT word) {
      return word.ends_with('b') &&
             word.find_first_not_of('a') == word.size() - 1;
    });
  }
}

TEST(RecognitionTest, StartMayBeAtSignNullOrHighBitByte) {
  for (CharT start : {'@', '\0', static_cast<CharT>(0xFF)}) {
    const auto grammar =
        Grammar::FromSpec({"a",
                           StringT{start},
                           {{start, StringT{'a', start}}, {start, ""}},
                           start})
            .value();
    for (std::size_t k : {1U, 2U, 3U}) {
      auto parser = Parser::Compile(grammar, k);
      ASSERT_TRUE(parser.has_value());
      ExpectLanguage(*parser, "ab", 4, [](StringViewT word) {
        return word.find_first_not_of('a') == StringViewT::npos;
      });
    }
  }
}

TEST(RecognitionTest, StructuredWhitespaceAndNullAreInputSymbols) {
  const StringT input{' ', '\t', '\0', static_cast<CharT>(0xFF)};
  const auto grammar =
      Grammar::FromSpec({input, "S", {{'S', input}}, 'S'}).value();
  for (std::size_t k : {1U, 2U, 3U}) {
    auto parser = Parser::Compile(grammar, k);
    ASSERT_TRUE(parser.has_value());
    EXPECT_TRUE(parser->Accepts(input));
    EXPECT_FALSE(parser->Accepts(""));
    EXPECT_FALSE(parser->Accepts(input.substr(0, 2)));
    EXPECT_FALSE(parser->Accepts(input + '\0'));
    EXPECT_FALSE(parser->Accepts(input.substr(1)));
  }
}

TEST(RecognitionTest, NoByteNeedsToBeReservedForAugmentation) {
  lrk_parser::GrammarSpec spec{"", "S", {}, 'S'};
  for (unsigned int value = 0;
       value <= std::numeric_limits<unsigned char>::max(); ++value) {
    const auto symbol = static_cast<CharT>(value);
    if (symbol == 'S') {
      continue;
    }
    spec.terminals.push_back(symbol);
    spec.rules.push_back({'S', StringT{symbol}});
  }
  const auto grammar = Grammar::FromSpec(spec).value();
  auto parser = Parser::Compile(grammar, 1);
  ASSERT_TRUE(parser.has_value());

  for (CharT symbol : spec.terminals) {
    EXPECT_TRUE(parser->Accepts(StringT{symbol}));
    EXPECT_FALSE(parser->Accepts(StringT{symbol, symbol}));
  }
  EXPECT_FALSE(parser->Accepts("S"));
  EXPECT_FALSE(parser->Accepts(""));
}

TEST(RecognitionTest, OwnsPreparedRulesAfterGrammarDestruction) {
  const auto parser = [] {
    const auto grammar =
        Grammar::FromSpec({"a", "S", {{'S', "aS"}, {'S', ""}}, 'S'});
    return Parser::Compile(grammar.value(), 2);
  }();
  ASSERT_TRUE(parser.has_value());

  ExpectLanguage(*parser, "ab", 4, [](StringViewT word) {
    return word.find_first_not_of('a') == StringViewT::npos;
  });
}

}  // namespace

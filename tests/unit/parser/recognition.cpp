#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "lrk_parser/parser.hpp"
#include "lrk_parser/text_grammar.hpp"

namespace {

using lrk_parser::CharT;
using lrk_parser::LrkParser;
using lrk_parser::MakeGrammar;
using lrk_parser::ParseGrammar;
using lrk_parser::StringT;
using lrk_parser::StringViewT;

template <typename Predicate>
void ExpectLanguage(const LrkParser& parser, StringViewT alphabet,
                    std::size_t max_length, Predicate accepts) {
  std::vector<StringT> words{StringT{}};
  for (std::size_t length = 0; length <= max_length; ++length) {
    std::vector<StringT> next_words;
    for (const auto& word : words) {
      EXPECT_EQ(parser.predict(word), accepts(word))
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
  const auto grammar = ParseGrammar("a", "S", {"S->"}, 'S').value();
  for (std::size_t k : {1U, 2U, 3U}) {
    SCOPED_TRACE(k);
    LrkParser parser;
    ASSERT_NO_THROW(parser.fit(grammar, k));
    ExpectLanguage(parser, "a", 4,
                   [](const StringT& word) { return word.empty(); });
  }
}

TEST(RecognitionTest, EqualNumbersOfAsAndBs) {
  const auto grammar = ParseGrammar("ab", "S", {"S->aSb", "S->"}, 'S').value();
  for (std::size_t k : {1U, 2U, 3U}) {
    SCOPED_TRACE(k);
    LrkParser parser;
    ASSERT_NO_THROW(parser.fit(grammar, k));
    ExpectLanguage(parser, "ab", 6, [](const StringT& word) {
      const auto half = word.size() / 2;
      return word == StringT(half, 'a') + StringT(half, 'b');
    });
  }
}

TEST(RecognitionTest, BalancedParentheses) {
  const auto grammar = ParseGrammar("()", "S", {"S->(S)S", "S->"}, 'S').value();
  for (std::size_t k : {1U, 2U, 3U}) {
    SCOPED_TRACE(k);
    LrkParser parser;
    ASSERT_NO_THROW(parser.fit(grammar, k));
    ExpectLanguage(parser, "()", 6, IsBalanced);
  }
}

TEST(RecognitionTest, LeftAndRightRecursion) {
  for (const auto* rule : {"S->Sa", "S->aS"}) {
    SCOPED_TRACE(rule);
    const auto grammar = ParseGrammar("ab", "S", {rule, "S->"}, 'S').value();
    LrkParser parser;
    ASSERT_NO_THROW(parser.fit(grammar, 1));
    ExpectLanguage(parser, "ab", 6, [](const StringT& word) {
      return word.find('b') == StringT::npos;
    });
  }
}

TEST(RecognitionTest, NullableChain) {
  const auto grammar =
      ParseGrammar("abc", "SABC",
                   {"S->ABC", "A->a", "A->", "B->b", "B->", "C->c", "C->"}, 'S')
          .value();
  const std::vector<StringT> language{"",   "a",  "b",  "c",
                                      "ab", "ac", "bc", "abc"};
  for (std::size_t k : {1U, 2U, 3U}) {
    SCOPED_TRACE(k);
    LrkParser parser;
    ASSERT_NO_THROW(parser.fit(grammar, k));
    ExpectLanguage(parser, "abc", 4, [&](const StringT& word) {
      return std::ranges::find(language, word) != language.end();
    });
  }
}

TEST(RecognitionTest, ExplicitLookaheadForLR2AndLR3Grammars) {
  for (std::size_t k : {2U, 3U}) {
    SCOPED_TRACE(k);
    const auto grammar =
        ParseGrammar("a", "SAB",
                     {"S->A" + StringT(k, 'a'), "S->B" + StringT(k - 1, 'a'),
                      "A->a", "B->a"},
                     'S')
            .value();
    LrkParser parser;
    for (std::size_t smaller_k = 1; smaller_k < k; ++smaller_k) {
      EXPECT_THROW(parser.fit(grammar, smaller_k), std::runtime_error);
    }
    ASSERT_NO_THROW(parser.fit(grammar, k));
    ExpectLanguage(parser, "ab", 5, [k](const StringT& word) {
      return word == StringT(k, 'a') || word == StringT(k + 1, 'a');
    });
  }
}

TEST(RecognitionTest, NonterminatingRecursionHasEmptyLanguage) {
  const auto grammar = ParseGrammar("a", "S", {"S->aS"}, 'S').value();
  for (std::size_t k : {1U, 2U, 3U}) {
    SCOPED_TRACE(k);
    LrkParser parser;
    ASSERT_NO_THROW(parser.fit(grammar, k));
    ExpectLanguage(parser, "a", 5, [](const StringT&) { return false; });
  }
}

TEST(RecognitionTest, ProductiveBranchBesideNonterminatingRecursion) {
  const auto grammar =
      ParseGrammar("ab", "SB", {"S->a", "S->B", "B->bB"}, 'S').value();
  for (std::size_t k : {1U, 2U, 3U}) {
    SCOPED_TRACE(k);
    LrkParser parser;
    ASSERT_NO_THROW(parser.fit(grammar, k));
    ExpectLanguage(parser, "ab", 5,
                   [](const StringT& word) { return word == "a"; });
  }
}

TEST(RecognitionTest, UnreachableConflictsDoNotAffectStartLanguage) {
  const std::vector<std::vector<StringT>> grammars{
      {"S->a", "A->b", "A->B", "B->b"}, {"S->a", "A->AA", "A->b"}};
  for (const auto& rules : grammars) {
    SCOPED_TRACE(::testing::PrintToString(rules));
    const auto grammar = ParseGrammar("ab", "SAB", rules, 'S').value();
    for (std::size_t k : {1U, 2U, 3U}) {
      SCOPED_TRACE(k);
      LrkParser parser;
      ASSERT_NO_THROW(parser.fit(grammar, k));
      ExpectLanguage(parser, "ab", 5,
                     [](const StringT& word) { return word == "a"; });
    }
  }
}

TEST(RecognitionTest, UnitCycleReportsConflictAtFixedLookahead) {
  const auto grammar = ParseGrammar("a", "S", {"S->S"}, 'S').value();
  for (std::size_t k : {1U, 2U, 3U}) {
    SCOPED_TRACE(k);
    LrkParser parser;
    EXPECT_THROW(parser.fit(grammar, k), std::runtime_error);
  }
}

TEST(RecognitionTest, CanonicalLR1StatesWithTheSameCoreStaySeparate) {
  const auto grammar = MakeGrammar({"abcde",
                                    "SAB",
                                    {{'S', "aAd"},
                                     {'S', "bAe"},
                                     {'S', "aBe"},
                                     {'S', "bBd"},
                                     {'A', "c"},
                                     {'B', "c"}},
                                    'S'})
                           .value();
  LrkParser parser;
  ASSERT_NO_THROW(parser.fit(grammar, 1));

  ExpectLanguage(parser, "abcde", 4, [](StringViewT word) {
    return word == "acd" || word == "ace" || word == "bcd" || word == "bce";
  });
}

TEST(RecognitionTest, AtSignIsAnOrdinaryTerminal) {
  const auto grammar = ParseGrammar("@", "S", {"S->@S", "S->"}, 'S').value();
  for (std::size_t k : {1U, 2U, 3U}) {
    LrkParser parser;
    ASSERT_NO_THROW(parser.fit(grammar, k));
    ExpectLanguage(parser, "@a", 4, [](StringViewT word) {
      return word.find_first_not_of('@') == StringViewT::npos;
    });
  }
}

TEST(RecognitionTest, AtSignNonterminalReducesBeforeAccept) {
  const auto grammar =
      ParseGrammar("ab", "S@", {"S->@b", "@->a@", "@->"}, 'S').value();
  for (std::size_t k : {1U, 2U, 3U}) {
    LrkParser parser;
    ASSERT_NO_THROW(parser.fit(grammar, k));
    ExpectLanguage(parser, "ab", 4, [](StringViewT word) {
      return word.ends_with('b') &&
             word.find_first_not_of('a') == word.size() - 1;
    });
  }
}

TEST(RecognitionTest, StartMayBeAtSignNullOrHighBitByte) {
  for (CharT start : {'@', '\0', static_cast<CharT>(0xFF)}) {
    const auto grammar =
        MakeGrammar({"a",
                     StringT{start},
                     {{start, StringT{'a', start}}, {start, ""}},
                     start})
            .value();
    for (std::size_t k : {1U, 2U, 3U}) {
      LrkParser parser;
      ASSERT_NO_THROW(parser.fit(grammar, k));
      ExpectLanguage(parser, "ab", 4, [](StringViewT word) {
        return word.find_first_not_of('a') == StringViewT::npos;
      });
    }
  }
}

TEST(RecognitionTest, StructuredWhitespaceAndNullAreInputSymbols) {
  const StringT input{' ', '\t', '\0', static_cast<CharT>(0xFF)};
  const auto grammar = MakeGrammar({input, "S", {{'S', input}}, 'S'}).value();
  for (std::size_t k : {1U, 2U, 3U}) {
    LrkParser parser;
    ASSERT_NO_THROW(parser.fit(grammar, k));
    EXPECT_TRUE(parser.predict(input));
    EXPECT_FALSE(parser.predict(""));
    EXPECT_FALSE(parser.predict(input.substr(0, 2)));
    EXPECT_FALSE(parser.predict(input + '\0'));
    EXPECT_FALSE(parser.predict(input.substr(1)));
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
  const auto grammar = MakeGrammar(spec).value();
  LrkParser parser;
  ASSERT_NO_THROW(parser.fit(grammar, 1));

  for (CharT symbol : spec.terminals) {
    EXPECT_TRUE(parser.predict(StringT{symbol}));
    EXPECT_FALSE(parser.predict(StringT{symbol, symbol}));
  }
  EXPECT_FALSE(parser.predict("S"));
  EXPECT_FALSE(parser.predict(""));
}

TEST(RecognitionTest, OwnsPreparedRulesAfterGrammarDestruction) {
  const auto parser = [] {
    const auto grammar = MakeGrammar({"a", "S", {{'S', "aS"}, {'S', ""}}, 'S'});
    LrkParser result;
    result.fit(grammar.value(), 2);
    return result;
  }();

  ExpectLanguage(parser, "ab", 4, [](StringViewT word) {
    return word.find_first_not_of('a') == StringViewT::npos;
  });
}

}  // namespace

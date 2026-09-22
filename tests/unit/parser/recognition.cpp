#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "lrk_parser/grammar.hpp"
#include "lrk_parser/parser.hpp"

namespace {

using lrk_parser::Grammar;
using lrk_parser::LrkParser;

template <typename Predicate>
void ExpectLanguage(const LrkParser& parser, std::string_view alphabet,
                    std::size_t max_length, Predicate accepts) {
  std::vector<std::string> words{std::string{}};
  for (std::size_t length = 0; length <= max_length; ++length) {
    std::vector<std::string> next_words;
    for (const auto& word : words) {
      EXPECT_EQ(parser.predict(word), accepts(word))
          << "word: '" << word << "'";
      if (length < max_length) {
        for (char symbol : alphabet) {
          next_words.push_back(word + symbol);
        }
      }
    }
    words = std::move(next_words);
  }
}

bool IsBalanced(std::string_view word) {
  std::size_t depth = 0;
  for (char symbol : word) {
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
  const auto grammar = Grammar::create_from_text("a", "S", {"S->"}, 'S');
  for (std::size_t k : {1U, 2U, 3U}) {
    SCOPED_TRACE(k);
    LrkParser parser;
    ASSERT_NO_THROW(parser.fit(grammar, k));
    ExpectLanguage(parser, "a", 4,
                   [](const std::string& word) { return word.empty(); });
  }
}

TEST(RecognitionTest, EqualNumbersOfAsAndBs) {
  const auto grammar =
      Grammar::create_from_text("ab", "S", {"S->aSb", "S->"}, 'S');
  for (std::size_t k : {1U, 2U, 3U}) {
    SCOPED_TRACE(k);
    LrkParser parser;
    ASSERT_NO_THROW(parser.fit(grammar, k));
    ExpectLanguage(parser, "ab", 6, [](const std::string& word) {
      const auto half = word.size() / 2;
      return word == std::string(half, 'a') + std::string(half, 'b');
    });
  }
}

TEST(RecognitionTest, BalancedParentheses) {
  const auto grammar =
      Grammar::create_from_text("()", "S", {"S->(S)S", "S->"}, 'S');
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
    const auto grammar =
        Grammar::create_from_text("ab", "S", {rule, "S->"}, 'S');
    LrkParser parser;
    ASSERT_NO_THROW(parser.fit(grammar, 1));
    ExpectLanguage(parser, "ab", 6, [](const std::string& word) {
      return word.find('b') == std::string::npos;
    });
  }
}

TEST(RecognitionTest, NullableChain) {
  const auto grammar = Grammar::create_from_text(
      "abc", "SABC", {"S->ABC", "A->a", "A->", "B->b", "B->", "C->c", "C->"},
      'S');
  const std::vector<std::string> language{"",   "a",  "b",  "c",
                                          "ab", "ac", "bc", "abc"};
  for (std::size_t k : {1U, 2U, 3U}) {
    SCOPED_TRACE(k);
    LrkParser parser;
    ASSERT_NO_THROW(parser.fit(grammar, k));
    ExpectLanguage(parser, "abc", 4, [&](const std::string& word) {
      return std::ranges::find(language, word) != language.end();
    });
  }
}

TEST(RecognitionTest, ExplicitLookaheadForLR2AndLR3Grammars) {
  for (std::size_t k : {2U, 3U}) {
    SCOPED_TRACE(k);
    const auto grammar = Grammar::create_from_text(
        "a", "SAB",
        {"S->A" + std::string(k, 'a'), "S->B" + std::string(k - 1, 'a'), "A->a",
         "B->a"},
        'S');
    LrkParser parser;
    for (std::size_t smaller_k = 1; smaller_k < k; ++smaller_k) {
      EXPECT_THROW(parser.fit(grammar, smaller_k), std::runtime_error);
    }
    ASSERT_NO_THROW(parser.fit(grammar, k));
    ExpectLanguage(parser, "ab", 5, [k](const std::string& word) {
      return word == std::string(k, 'a') || word == std::string(k + 1, 'a');
    });
  }
}

TEST(RecognitionTest, NonterminatingRecursionHasEmptyLanguage) {
  const auto grammar = Grammar::create_from_text("a", "S", {"S->aS"}, 'S');
  for (std::size_t k : {1U, 2U, 3U}) {
    SCOPED_TRACE(k);
    LrkParser parser;
    ASSERT_NO_THROW(parser.fit(grammar, k));
    ExpectLanguage(parser, "a", 5, [](const std::string&) { return false; });
  }
}

TEST(RecognitionTest, ProductiveBranchBesideNonterminatingRecursion) {
  const auto grammar =
      Grammar::create_from_text("ab", "SB", {"S->a", "S->B", "B->bB"}, 'S');
  for (std::size_t k : {1U, 2U, 3U}) {
    SCOPED_TRACE(k);
    LrkParser parser;
    ASSERT_NO_THROW(parser.fit(grammar, k));
    ExpectLanguage(parser, "ab", 5,
                   [](const std::string& word) { return word == "a"; });
  }
}

TEST(RecognitionTest, UnreachableConflictsDoNotAffectStartLanguage) {
  const std::vector<std::vector<std::string>> grammars{
      {"S->a", "A->b", "A->B", "B->b"}, {"S->a", "A->AA", "A->b"}};
  for (const auto& rules : grammars) {
    SCOPED_TRACE(::testing::PrintToString(rules));
    const auto grammar = Grammar::create_from_text("ab", "SAB", rules, 'S');
    for (std::size_t k : {1U, 2U, 3U}) {
      SCOPED_TRACE(k);
      LrkParser parser;
      ASSERT_NO_THROW(parser.fit(grammar, k));
      ExpectLanguage(parser, "ab", 5,
                     [](const std::string& word) { return word == "a"; });
    }
  }
}

TEST(RecognitionTest, UnitCycleReportsConflictAtFixedLookahead) {
  const auto grammar = Grammar::create_from_text("a", "S", {"S->S"}, 'S');
  for (std::size_t k : {1U, 2U, 3U}) {
    SCOPED_TRACE(k);
    LrkParser parser;
    EXPECT_THROW(parser.fit(grammar, k), std::runtime_error);
  }
}

}  // namespace

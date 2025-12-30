#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "lrk_parser/parser.hpp"

using namespace lrk_parser;
using namespace lrk_parser::details;

class LrkParserTest : public ::testing::Test {
 protected:
  LrkParser parser_;

  void InitParser(const StringT& terminals, const StringT& non_terminals,
                  const VectorT<StringT>& rules, CharT start_symbol,
                  std::size_t k) {
    Grammar grammar = Grammar::create_from_text(terminals, non_terminals, rules,
                                                start_symbol);
    parser_.fit(std::move(grammar), k);
  }

  void InitParser(const StringT& terminals, const StringT& non_terminals,
                  const VectorT<StringT>& rules, CharT start_symbol) {
    Grammar grammar = Grammar::create_from_text(terminals, non_terminals, rules,
                                                start_symbol);
    parser_.fit(std::move(grammar));
  }
};

TEST_F(LrkParserTest, BasicLR1SuccessAndFailure) {
  StringT T = "id=";
  StringT N = "E";
  CharT Start = 'E';
  VectorT<StringT> rules = {"E->id=E", "E->id"};

  ASSERT_NO_THROW({ InitParser(T, N, rules, Start, 1); });

  EXPECT_TRUE(parser_.predict("id=id")) << "Failed to parse 'id=id'";

  EXPECT_TRUE(parser_.predict("id")) << "Failed to parse 'id'";

  EXPECT_FALSE(parser_.predict("id=")) << "Incorrectly parsed 'id='";

  EXPECT_FALSE(parser_.predict("id=id=")) << "Incorrectly parsed 'id=id='";

  ASSERT_NO_THROW({ InitParser(T, N, rules, Start); });
  
  EXPECT_TRUE(parser_.predict("id=id")) << "Failed to parse 'id=id'";

  EXPECT_TRUE(parser_.predict("id")) << "Failed to parse 'id'";

  EXPECT_FALSE(parser_.predict("id=")) << "Incorrectly parsed 'id='";

  EXPECT_FALSE(parser_.predict("id=id=")) << "Incorrectly parsed 'id=id='";
}

TEST_F(LrkParserTest, NestedStructureGrammar) {
  StringT T = "ab";
  StringT N = "S";
  CharT Start = 'S';
  VectorT<StringT> rules = {"S->aSbS", "S->"};

  ASSERT_NO_THROW({ InitParser(T, N, rules, Start, 1); });

  EXPECT_TRUE(parser_.predict("aababb")) << "Failed to parse 'aababb'";

  EXPECT_FALSE(parser_.predict("aabbba")) << "Incorrectly parsed 'aabbba'";

  EXPECT_TRUE(parser_.predict("ab")) << "Failed to parse 'ab'";

  EXPECT_TRUE(parser_.predict("")) << "Failed to parse empty string (ε)";
}

TEST_F(LrkParserTest, LR2Necessity) {
  StringT T = "a";
  StringT N = "SAB";
  CharT Start = 'S';
  VectorT<StringT> rules = {"S->Aaa", "S->Ba", "A->a", "B->a"};

  EXPECT_THROW(
      { InitParser(T, N, rules, Start, 1); }, std::runtime_error)
      << "ActionTable was built successfully with k=1, expected R/R conflict "
         "to fail compilation.";

  ASSERT_NO_THROW({ InitParser(T, N, rules, Start, 2); })
      << "ActionTable failed to build even with k=2.";

  EXPECT_TRUE(parser_.predict("aaa")) << "Failed to parse 'aaa' with k=2";

  EXPECT_TRUE(parser_.predict("aa")) << "Failed to parse 'aa' with k=2";

  EXPECT_FALSE(parser_.predict("a")) << "Incorrectly parsed 'a' with k=2";
}

TEST_F(LrkParserTest, WorksWithK3) {
  StringT T = "a";
  StringT N = "SAB";
  CharT Start = 'S';
  VectorT<StringT> rules = {"S->Aaaa", "S->Baa", "A->a", "B->a"};

  ASSERT_THROW(
      { InitParser(T, N, rules, Start, 1); }, std::runtime_error)
      << "ActionTable was built successfully with k=1, expected R/R conflict "
         "to fail compilation.";

  ASSERT_THROW(
      { InitParser(T, N, rules, Start, 2); }, std::runtime_error)
      << "ActionTable was built successfully with k=2, expected R/R conflict "
         "to fail compilation.";

  ASSERT_NO_THROW({ InitParser(T, N, rules, Start, 3); })
      << "ActionTable failed to build with k=3.";

  EXPECT_TRUE(parser_.predict("aaaa")) << "Failed to parse 'aaaa' with k=3";

  EXPECT_TRUE(parser_.predict("aaa")) << "Failed to parse 'aaa' with k=3";

  EXPECT_FALSE(parser_.predict("aa")) << "Incorrectly parsed 'aa' with k=3";
}

TEST_F(LrkParserTest, TrivialLR0Success) {
  StringT T = "ab";
  StringT N = "E";
  CharT Start = 'E';
  VectorT<StringT> rules = {"E->a"};

  ASSERT_NO_THROW({ InitParser(T, N, rules, Start, 0); })
      << "ActionTable failed to build for a Trivial LR(0) grammar with k=0.";

  EXPECT_TRUE(parser_.predict("a")) << "Failed to parse 'a' with k=0";

  EXPECT_FALSE(parser_.predict(""))
      << "Incorrectly parsed empty string with k=0";

  EXPECT_FALSE(parser_.predict("aa")) << "Incorrectly parsed 'aa' with k=0";
  EXPECT_FALSE(parser_.predict("b")) << "Incorrectly parsed 'b' with k=0";
}
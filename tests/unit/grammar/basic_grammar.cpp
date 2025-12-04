#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "lrk_parser/config.hpp"
#include "lrk_parser/grammar.hpp"
#include "lrk_parser/rule.hpp"

namespace lrk_parser {

using Grammar = details::Grammar;
using details::Rule;

struct GrammarTest : public ::testing::Test {
  StringT T = "abc";
  StringT N = "SAB";
  VectorT<StringT> RulesStr = {"S->AB", "A->a", "A->", "B->bB", "B->c"};
  CharT StartSym = 'S';
};

TEST_F(GrammarTest, InitWithStrs_ValidGrammar) {
  Grammar grammar = Grammar::init_with_strs(T, N, RulesStr, StartSym);

  EXPECT_EQ(grammar.get_terminals().size(), 3);
  EXPECT_TRUE(grammar.is_terminal('a'));
  EXPECT_FALSE(grammar.is_terminal('S'));

  EXPECT_EQ(grammar.get_nonterminals().size(), 3);
  EXPECT_TRUE(grammar.is_nonterminal('S'));
  EXPECT_FALSE(grammar.is_nonterminal('a'));

  const auto& rules = grammar.get_rules();
  EXPECT_EQ(rules.size(), 6);

  EXPECT_EQ(rules[0].lhs, '@');
  EXPECT_EQ(rules[0].rhs, "S");

  EXPECT_TRUE(grammar.is_rules_exists('A'));
  const auto& a_rules_idxs = grammar.get_rules_idxs('A');
  EXPECT_EQ(a_rules_idxs.size(), 2);
  EXPECT_EQ(grammar.get_rule_by_idx(a_rules_idxs[0]).rhs, "a");
  EXPECT_EQ(grammar.get_rule_by_idx(a_rules_idxs[1]).rhs, "");

  EXPECT_TRUE(grammar.is_valid_symbol('a'));
  EXPECT_TRUE(grammar.is_valid_symbol('S'));
  EXPECT_FALSE(grammar.is_valid_symbol('Z'));
  EXPECT_FALSE(grammar.is_valid_symbol('@'));
}

TEST_F(GrammarTest, InitWithStrs_PrepareRulesStr_RemovesSpaces) {
  VectorT<StringT> rules_with_spaces = {" S -> AB ", "A -> a"};
  Grammar grammar = Grammar::init_with_strs(T, N, rules_with_spaces, StartSym);
  const auto& rules = grammar.get_rules();

  EXPECT_EQ(rules[1].lhs, 'S');
  EXPECT_EQ(rules[1].rhs, "AB");
  EXPECT_EQ(rules[2].lhs, 'A');
  EXPECT_EQ(rules[2].rhs, "a");
}

TEST_F(GrammarTest, InitWithStrs_Throws_ReservedStarSymbolInTerminals) {
  EXPECT_THROW(
      std::ignore = Grammar::init_with_strs(T + '@', N, RulesStr, StartSym),
      std::logic_error);
}

TEST_F(GrammarTest, InitWithStrs_Throws_ReservedStarSymbolInNonTerminals) {
  EXPECT_THROW(
      std::ignore = Grammar::init_with_strs(T, N + '@', RulesStr, StartSym),
      std::logic_error);
}

TEST_F(GrammarTest, InitWithStrs_Throws_OverlappingSets) {
  EXPECT_THROW(
      std::ignore = Grammar::init_with_strs(T, N + 'a', RulesStr, StartSym),
      std::logic_error);
}

TEST_F(GrammarTest, InitWithStrs_Throws_RuleTooShort) {
  VectorT<StringT> bad_rules = {"S-"};
  EXPECT_THROW(std::ignore = Grammar::init_with_strs(T, N, bad_rules, StartSym),
               std::logic_error);
}

TEST_F(GrammarTest, InitWithStrs_Throws_ArrowMissing) {
  VectorT<StringT> bad_rules = {"S>AB"};
  EXPECT_THROW(std::ignore = Grammar::init_with_strs(T, N, bad_rules, StartSym),
               std::logic_error);
}

TEST_F(GrammarTest, InitWithStrs_Throws_ArrowWrongPos) {
  VectorT<StringT> bad_rules = {"AS->B"};
  EXPECT_THROW(std::ignore = Grammar::init_with_strs(T, N, bad_rules, StartSym),
               std::logic_error);
}

TEST_F(GrammarTest, InitWithStrs_Throws_TerminalOnLHS) {
  VectorT<StringT> bad_rules = {"a->B"};
  EXPECT_THROW(std::ignore = Grammar::init_with_strs(T, N, bad_rules, StartSym),
               std::logic_error);
}

TEST_F(GrammarTest, InitWithStrs_Throws_UnknownSymbolOnLHS) {
  VectorT<StringT> bad_rules = {"Z->AB"};
  EXPECT_THROW(std::ignore = Grammar::init_with_strs(T, N, bad_rules, StartSym),
               std::logic_error);
}

TEST_F(GrammarTest, InitWithStrs_Throws_UnknownSymbolOnRHS) {
  VectorT<StringT> bad_rules = {"S->Z"};
  EXPECT_THROW(std::ignore = Grammar::init_with_strs(T, N, bad_rules, StartSym),
               std::logic_error);
}

TEST_F(GrammarTest, Accessors_Getters) {
  Grammar grammar = Grammar::init_with_strs(T, N, RulesStr, StartSym);

  EXPECT_TRUE(grammar.is_rules_exists('S'));
  EXPECT_FALSE(grammar.is_rules_exists('Z'));
  EXPECT_THROW(std::ignore = grammar.get_rules_idxs('Z'), std::out_of_range);

  const auto& s_rule_idxs = grammar.get_rules_idxs('S');
  EXPECT_EQ(s_rule_idxs.size(), 1);

  const auto& rule = grammar.get_rule_by_idx(s_rule_idxs[0]);
  EXPECT_EQ(rule.lhs, 'S');
  EXPECT_EQ(rule.rhs, "AB");

  EXPECT_EQ(grammar.get_rules().size(), 6);

  EXPECT_TRUE(grammar.get_terminals().count('a'));
  EXPECT_TRUE(grammar.get_nonterminals().count('S'));
}

TEST(GrammarTestSimple, ConstructorsAndAssignments) {
  Grammar g1;

  Grammar g2 = g1;
  g2 = g1;
  Grammar g3 = std::move(g1);
  g3 = std::move(g2);

  SUCCEED();
}

}  // namespace lrk_parser
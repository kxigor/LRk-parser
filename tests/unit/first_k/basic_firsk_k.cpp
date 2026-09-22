#include <gtest/gtest.h>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "lrk_parser/config.hpp"
#include "lrk_parser/first_k.hpp"
#include "lrk_parser/rule.hpp"
#include "lrk_parser/text_grammar.hpp"

namespace lrk_parser::details {
class FirstKTestAccessor : public FirstK {
 public:
  FirstKTestAccessor(const PreparedGrammar& grammar, const std::size_t& k)
      : FirstK(grammar, k) {}

  static void PublicUnionKSets(UsetT<StringT>& lhs_set,
                               const UsetT<StringT>& rhs_set) {
    FirstK::union_k_sets(lhs_set, rhs_set);
  }

  UsetT<StringT> PublicConcatKSets(const UsetT<StringT>& lhs_set,
                                   const UsetT<StringT>& rhs_set) const {
    return concat_k_sets(lhs_set, rhs_set);
  }

  const UmapT<SymbolId, UsetT<StringT>>& GetFirstKMap() const {
    return first_k_;
  }
};
}  // namespace lrk_parser::details

using namespace lrk_parser;
using namespace lrk_parser::details;

struct FirstKTestGrammar : public ::testing::Test {
  StringT T = "abcd";
  StringT N = "SAB";
  CharT StartSym = 'S';

  VectorT<StringT> RulesStr_K1 = {"S->AB", "S->c", "A->a",
                                  "A->",   "B->b", "B->"};

  VectorT<StringT> RulesStr_K2 = {"S->Aa", "A->Bc", "B->d", "B->"};

  PreparedGrammar create_grammar(const VectorT<StringT>& rules) {
    return PreparedGrammar(ParseGrammar(T, N, rules, StartSym).value());
  }
};

TEST_F(FirstKTestGrammar, UnionKSets_Private) {
  UsetT<StringT> lhs = {"a", "b", "c"};
  const UsetT<StringT> rhs = {"b", "d", "e"};

  FirstKTestAccessor::PublicUnionKSets(lhs, rhs);

  EXPECT_EQ(lhs.size(), 5);
  EXPECT_TRUE(lhs.count("a"));
  EXPECT_TRUE(lhs.count("b"));
  EXPECT_TRUE(lhs.count("c"));
  EXPECT_TRUE(lhs.count("d"));
  EXPECT_TRUE(lhs.count("e"));
}

TEST_F(FirstKTestGrammar, ConcatKSets_K2_Truncation) {
  PreparedGrammar g = create_grammar({"S->"});
  FirstKTestAccessor fk_acc(g, 2);

  UsetT<StringT> set1 = {"ab", "a"};
  UsetT<StringT> set2 = {"c", ""};

  UsetT<StringT> expected = {"ab", "ac", "a"};
  UsetT<StringT> result = fk_acc.PublicConcatKSets(set1, set2);

  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result, expected);
}

TEST_F(FirstKTestGrammar, ConcatKSets_K3_FullTruncation) {
  PreparedGrammar g = create_grammar({"S->"});
  FirstKTestAccessor fk_acc(g, 3);

  UsetT<StringT> set1 = {"a", "b"};
  UsetT<StringT> set2 = {"cde", "fg"};

  UsetT<StringT> expected = {"acd", "afg", "bcd", "bfg"};
  UsetT<StringT> result = fk_acc.PublicConcatKSets(set1, set2);

  EXPECT_EQ(result.size(), 4);
  EXPECT_EQ(result, expected);
}

TEST_F(FirstKTestGrammar, FullFirstKComputation_K1) {
  PreparedGrammar g = create_grammar(RulesStr_K1);
  FirstKTestAccessor fk_acc(g, 1);

  const auto& first_k_map = fk_acc.GetFirstKMap();

  EXPECT_EQ(first_k_map.at(EncodeSymbol('a')), (UsetT<StringT>{"a"}));
  EXPECT_EQ(first_k_map.at(EncodeSymbol('b')), (UsetT<StringT>{"b"}));
  EXPECT_EQ(first_k_map.at(EncodeSymbol('c')), (UsetT<StringT>{"c"}));

  EXPECT_EQ(first_k_map.at(EncodeSymbol('A')).size(), 2);
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('A')).count("a"));
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('A')).count(""));

  EXPECT_EQ(first_k_map.at(EncodeSymbol('B')).size(), 2);
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('B')).count("b"));
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('B')).count(""));

  EXPECT_EQ(first_k_map.at(EncodeSymbol('S')).size(), 4);
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S')).count("a"));
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S')).count("b"));
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S')).count("c"));
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S')).count(""));
}

TEST_F(FirstKTestGrammar, FullFirstKComputation_K2) {
  PreparedGrammar g = create_grammar(RulesStr_K2);
  FirstKTestAccessor fk_acc(g, 2);

  const auto& first_k_map = fk_acc.GetFirstKMap();

  EXPECT_EQ(first_k_map.at(EncodeSymbol('B')).size(), 2);
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('B')).count("d"));
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('B')).count(""));

  EXPECT_EQ(first_k_map.at(EncodeSymbol('A')).size(), 2);
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('A')).count("dc"));
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('A')).count("c"));

  EXPECT_EQ(first_k_map.at(EncodeSymbol('S')).size(), 2);
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S')).count("dc"));
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S')).count("ca"));
}

TEST_F(FirstKTestGrammar, ComputeFirstKForString_K3) {
  VectorT<StringT> rules = {
      {"S->ABa"}, {"A->a"}, {"A->Bc"}, {"B->c"}, {"B->"},
  };
  PreparedGrammar g = create_grammar(rules);
  FirstK fk(g, 3);

  UsetT<StringT> expected = {"aa", "aca", "ccc", "ca", "cca"};
  UsetT<StringT> result = fk.compute_first_k(EncodeSymbols("ABa"));

  EXPECT_EQ(result, expected);
}

TEST_F(FirstKTestGrammar, FullFirstKComputation_K3) {
  StringT terminals = "abcde";
  StringT non_terminals = "SABCD";
  CharT start_sym = 'S';

  VectorT<StringT> rules = {
      {"S->ABC"}, {"A->a"},  {"A->BD"}, {"B->b"}, {"B->"},
      {"C->c"},   {"C->De"}, {"D->d"},  {"D->"},
  };

  PreparedGrammar g = PreparedGrammar(
      ParseGrammar(terminals, non_terminals, rules, start_sym).value());
  FirstKTestAccessor fk_acc(g, 3);

  const auto& first_k_map = fk_acc.GetFirstKMap();

  EXPECT_EQ(first_k_map.at(EncodeSymbol('a')).size(), 1);
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('a')).count("a"));

  EXPECT_EQ(first_k_map.at(EncodeSymbol('b')).size(), 1);
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('b')).count("b"));

  EXPECT_EQ(first_k_map.at(EncodeSymbol('c')).size(), 1);
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('c')).count("c"));

  EXPECT_EQ(first_k_map.at(EncodeSymbol('d')).size(), 1);
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('d')).count("d"));

  EXPECT_EQ(first_k_map.at(EncodeSymbol('e')).size(), 1);
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('e')).count("e"));

  EXPECT_EQ(first_k_map.at(EncodeSymbol('D')).size(), 2);
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('D')).count("d"));
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('D')).count(""));

  EXPECT_EQ(first_k_map.at(EncodeSymbol('B')).size(), 2);
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('B')).count("b"));
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('B')).count(""));

  EXPECT_EQ(first_k_map.at(EncodeSymbol('C')).size(), 3);
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('C')).count("c"));
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('C')).count("e"));   // от D→ε
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('C')).count("de"));  // от D→d

  EXPECT_EQ(first_k_map.at(EncodeSymbol('A')).size(), 5);
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('A')).count("a"));   // от A→a
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('A')).count(""));    // от B→ε, D→ε
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('A')).count("d"));   // от B→ε, D→d
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('A')).count("b"));   // от B→b, D→ε
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('A')).count("bd"));  // от B→b, D→d

  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S')).count("ac"));  // A→a, B→ε, C→c
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S'))
                  .count("ae"));  // A→a, B→ε, C→e (через D→ε)
  EXPECT_TRUE(
      first_k_map.at(EncodeSymbol('S')).count("ade"));  // A→a, B→ε, C→de (D→d)
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S')).count("abc"));  // A→a, B→b, C→c
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S')).count("abe"));  // A→a, B→b, C→e
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S'))
                  .count("abd"));  // A→a, B→b, C→de → "abde" → "abd"

  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S')).count("c"));   // A→ε, B→ε, C→c
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S')).count("e"));   // A→ε, B→ε, C→e
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S')).count("de"));  // A→ε, B→ε, C→de
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S')).count("bc"));  // A→ε, B→b, C→c
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S')).count("be"));  // A→ε, B→b, C→e
  EXPECT_TRUE(
      first_k_map.at(EncodeSymbol('S')).count("bde"));  // A→ε, B→b, C→de

  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S')).count("bbc"));  // A→b, B→b, C→c
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S')).count("bbe"));  // A→b, B→b, C→e
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S'))
                  .count("bbd"));  // A→b, B→b, C→de → "bbde" → "bbd"

  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S')).count("dc"));  // A→d, B→ε, C→c
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S'))
                  .count("dde"));  // A→d, B→ε, C→de → "dde"
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S')).count("dbc"));  // A→d, B→b, C→c
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S')).count("dbe"));  // A→d, B→b, C→e
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S'))
                  .count("dbd"));  // A→d, B→b, C→de → "dbde" → "dbd"

  EXPECT_TRUE(
      first_k_map.at(EncodeSymbol('S')).count("bdc"));  // A→bd, B→ε, C→c
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S'))
                  .count("bdd"));  // A→bd, B→ε, C→de → "bdde" → "bdd"
  EXPECT_TRUE(first_k_map.at(EncodeSymbol('S'))
                  .count("bdb"));  // A→bd, B→b, C→c → "bdbc" → "bdb"

  EXPECT_FALSE(first_k_map.at(EncodeSymbol('S')).count("a"));
  EXPECT_FALSE(first_k_map.at(EncodeSymbol('S')).count("b"));
  EXPECT_FALSE(first_k_map.at(EncodeSymbol('S')).count("d"));

  EXPECT_EQ(first_k_map.at(EncodeSymbol('S')).size(), 23);
}
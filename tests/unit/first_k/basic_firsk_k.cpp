#include <gtest/gtest.h>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "lrk_parser/config.hpp"
#include "lrk_parser/first_k.hpp"
#include "lrk_parser/grammar.hpp"
#include "lrk_parser/rule.hpp"

namespace lrk_parser::details {
class FirstKTestAccessor : public FirstK {
 public:
  FirstKTestAccessor(const Grammar& grammar, const std::size_t& k)
      : FirstK(grammar, k) {}

  static void PublicUnionKSets(UsetT<StringT>& lhs_set,
                               const UsetT<StringT>& rhs_set) {
    FirstK::union_k_sets(lhs_set, rhs_set);
  }

  UsetT<StringT> PublicConcatKSets(const UsetT<StringT>& lhs_set,
                                   const UsetT<StringT>& rhs_set) const {
    // Внутри Accessor мы можем вызвать приватный метод, используя k_ из объекта
    return concat_k_sets(lhs_set, rhs_set);
  }

  const UmapT<CharT, UsetT<StringT>>& GetFirstKMap() const { return first_k_; }
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

  Grammar create_grammar(const VectorT<StringT>& rules) {
    return Grammar::init_with_strs(T, N, rules, StartSym);
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
  Grammar g = create_grammar({});
  FirstKTestAccessor fk_acc(g, 2);

  UsetT<StringT> set1 = {"ab", "a"};
  UsetT<StringT> set2 = {"c", ""};

  // Ожидается:
  // "ab" (длина >= k) -> "ab"
  // "a" + "c" -> "ac"
  // "a" + "" -> "a"
  // Результат: {"ab", "ac", "a"}
  UsetT<StringT> expected = {"ab", "ac", "a"};
  UsetT<StringT> result = fk_acc.PublicConcatKSets(set1, set2);

  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result, expected);
}

TEST_F(FirstKTestGrammar, ConcatKSets_K3_FullTruncation) {
  Grammar g = create_grammar({});
  FirstKTestAccessor fk_acc(g, 3);

  UsetT<StringT> set1 = {"a", "b"};
  UsetT<StringT> set2 = {"cde", "fg"};

  // Ожидается (k=3):
  // "a" + "cde" -> "acde" -> "acd" (trunc)
  // "a" + "fg" -> "afg"
  // "b" + "cde" -> "bcde" -> "bcd" (trunc)
  // "b" + "fg" -> "bfg"
  UsetT<StringT> expected = {"acd", "afg", "bcd", "bfg"};
  UsetT<StringT> result = fk_acc.PublicConcatKSets(set1, set2);

  EXPECT_EQ(result.size(), 4);
  EXPECT_EQ(result, expected);
}

TEST_F(FirstKTestGrammar, FullFirstKComputation_K1) {
  Grammar g = create_grammar(RulesStr_K1);
  FirstKTestAccessor fk_acc(g, 1);  // k = 1

  const auto& first_k_map = fk_acc.GetFirstKMap();

  EXPECT_EQ(first_k_map.at('a'), (UsetT<StringT>{"a"}));
  EXPECT_EQ(first_k_map.at('b'), (UsetT<StringT>{"b"}));
  EXPECT_EQ(first_k_map.at('c'), (UsetT<StringT>{"c"}));

  // First1(A) = {a, ""}
  EXPECT_EQ(first_k_map.at('A').size(), 2);
  EXPECT_TRUE(first_k_map.at('A').count("a"));
  EXPECT_TRUE(first_k_map.at('A').count(""));

  // First1(B) = {b, ""}
  EXPECT_EQ(first_k_map.at('B').size(), 2);
  EXPECT_TRUE(first_k_map.at('B').count("b"));
  EXPECT_TRUE(first_k_map.at('B').count(""));

  // First1(S) = First1(AB) U First1(c) = {a, b, c, ""}
  EXPECT_EQ(first_k_map.at('S').size(), 4);
  EXPECT_TRUE(first_k_map.at('S').count("a"));
  EXPECT_TRUE(first_k_map.at('S').count("b"));
  EXPECT_TRUE(first_k_map.at('S').count("c"));
  EXPECT_TRUE(first_k_map.at('S').count(""));
}

TEST_F(FirstKTestGrammar, FullFirstKComputation_K2) {
  // 1. Инициализация
  Grammar g = create_grammar(RulesStr_K2);
  FirstKTestAccessor fk_acc(g, 2);  // k = 2

  const auto& first_k_map = fk_acc.GetFirstKMap();

  // 2. Проверка First2(B) = {d, ""}
  EXPECT_EQ(first_k_map.at('B').size(), 2);
  EXPECT_TRUE(first_k_map.at('B').count("d"));
  EXPECT_TRUE(first_k_map.at('B').count(""));

  // 3. Проверка First2(A): A -> Bc
  // First2(B) * First2(c) = {d, ""} * {c} = {"dc", "c"}
  EXPECT_EQ(first_k_map.at('A').size(), 2);
  EXPECT_TRUE(first_k_map.at('A').count("dc"));
  EXPECT_TRUE(first_k_map.at('A').count("c"));

  // 4. Проверка First2(S): S -> Aa
  // First2(A) * First2(a) = {"dc", "c"} * {a} = {"dca"-> "dc", "ca"}
  EXPECT_EQ(first_k_map.at('S').size(), 2);
  EXPECT_TRUE(first_k_map.at('S').count("dc"));  // 'dca' усекается до 'dc'
  EXPECT_TRUE(first_k_map.at('S').count("ca"));
}

TEST_F(FirstKTestGrammar, ComputeFirstKForString_K3) {
  VectorT<StringT> rules = {
      {"A->a"},
      {"A->Bc"},
      {"B->c"},
      {"B->"},
  };
  Grammar g = create_grammar(rules);
  FirstK fk(g, 3);  // k = 3

  UsetT<StringT> expected = {"aa", "aca", "ccc", "ca", "cca"};
  UsetT<StringT> result = fk.compute_first_k("ABa");

  EXPECT_EQ(result, expected);
}

TEST_F(FirstKTestGrammar, FullFirstKComputation_K3) {
  // Подготовка грамматики
  StringT terminals = "abcde";
  StringT non_terminals = "SABCD";
  CharT start_sym = 'S';

  VectorT<StringT> rules = {
      {"S->ABC"}, {"A->a"},  {"A->BD"}, {"B->b"}, {"B->"},
      {"C->c"},   {"C->De"}, {"D->d"},  {"D->"},
  };

  Grammar g =
      Grammar::init_with_strs(terminals, non_terminals, rules, start_sym);
  FirstKTestAccessor fk_acc(g, 3);  // k = 3

  const auto& first_k_map = fk_acc.GetFirstKMap();

  // Проверка First₃ терминалов
  EXPECT_EQ(first_k_map.at('a').size(), 1);
  EXPECT_TRUE(first_k_map.at('a').count("a"));

  EXPECT_EQ(first_k_map.at('b').size(), 1);
  EXPECT_TRUE(first_k_map.at('b').count("b"));

  EXPECT_EQ(first_k_map.at('c').size(), 1);
  EXPECT_TRUE(first_k_map.at('c').count("c"));

  EXPECT_EQ(first_k_map.at('d').size(), 1);
  EXPECT_TRUE(first_k_map.at('d').count("d"));

  EXPECT_EQ(first_k_map.at('e').size(), 1);
  EXPECT_TRUE(first_k_map.at('e').count("e"));

  EXPECT_EQ(first_k_map.at('D').size(), 2);
  EXPECT_TRUE(first_k_map.at('D').count("d"));
  EXPECT_TRUE(first_k_map.at('D').count(""));

  EXPECT_EQ(first_k_map.at('B').size(), 2);
  EXPECT_TRUE(first_k_map.at('B').count("b"));
  EXPECT_TRUE(first_k_map.at('B').count(""));

  EXPECT_EQ(first_k_map.at('C').size(), 3);
  EXPECT_TRUE(first_k_map.at('C').count("c"));
  EXPECT_TRUE(first_k_map.at('C').count("e"));   // от D→ε
  EXPECT_TRUE(first_k_map.at('C').count("de"));  // от D→d

  EXPECT_EQ(first_k_map.at('A').size(), 5);
  EXPECT_TRUE(first_k_map.at('A').count("a"));   // от A→a
  EXPECT_TRUE(first_k_map.at('A').count(""));    // от B→ε, D→ε
  EXPECT_TRUE(first_k_map.at('A').count("d"));   // от B→ε, D→d
  EXPECT_TRUE(first_k_map.at('A').count("b"));   // от B→b, D→ε
  EXPECT_TRUE(first_k_map.at('A').count("bd"));  // от B→b, D→d

  EXPECT_TRUE(first_k_map.at('S').count("ac"));   // A→a, B→ε, C→c
  EXPECT_TRUE(first_k_map.at('S').count("ae"));   // A→a, B→ε, C→e (через D→ε)
  EXPECT_TRUE(first_k_map.at('S').count("ade"));  // A→a, B→ε, C→de (D→d)
  EXPECT_TRUE(first_k_map.at('S').count("abc"));  // A→a, B→b, C→c
  EXPECT_TRUE(first_k_map.at('S').count("abe"));  // A→a, B→b, C→e
  EXPECT_TRUE(
      first_k_map.at('S').count("abd"));  // A→a, B→b, C→de → "abde" → "abd"

  EXPECT_TRUE(first_k_map.at('S').count("c"));    // A→ε, B→ε, C→c
  EXPECT_TRUE(first_k_map.at('S').count("e"));    // A→ε, B→ε, C→e
  EXPECT_TRUE(first_k_map.at('S').count("de"));   // A→ε, B→ε, C→de
  EXPECT_TRUE(first_k_map.at('S').count("bc"));   // A→ε, B→b, C→c
  EXPECT_TRUE(first_k_map.at('S').count("be"));   // A→ε, B→b, C→e
  EXPECT_TRUE(first_k_map.at('S').count("bde"));  // A→ε, B→b, C→de

  EXPECT_TRUE(first_k_map.at('S').count("bbc"));  // A→b, B→b, C→c
  EXPECT_TRUE(first_k_map.at('S').count("bbe"));  // A→b, B→b, C→e
  EXPECT_TRUE(
      first_k_map.at('S').count("bbd"));  // A→b, B→b, C→de → "bbde" → "bbd"

  EXPECT_TRUE(first_k_map.at('S').count("dc"));   // A→d, B→ε, C→c
  EXPECT_TRUE(first_k_map.at('S').count("dde"));  // A→d, B→ε, C→de → "dde"
  EXPECT_TRUE(first_k_map.at('S').count("dbc"));  // A→d, B→b, C→c
  EXPECT_TRUE(first_k_map.at('S').count("dbe"));  // A→d, B→b, C→e
  EXPECT_TRUE(
      first_k_map.at('S').count("dbd"));  // A→d, B→b, C→de → "dbde" → "dbd"

  EXPECT_TRUE(first_k_map.at('S').count("bdc"));  // A→bd, B→ε, C→c
  EXPECT_TRUE(
      first_k_map.at('S').count("bdd"));  // A→bd, B→ε, C→de → "bdde" → "bdd"
  EXPECT_TRUE(
      first_k_map.at('S').count("bdb"));  // A→bd, B→b, C→c → "bdbc" → "bdb"

  EXPECT_FALSE(first_k_map.at('S').count("a"));  // C не может быть ε!
  EXPECT_FALSE(first_k_map.at('S').count("b"));  // C не может быть ε!
  EXPECT_FALSE(first_k_map.at('S').count("d"));  // C не может быть ε!

  EXPECT_EQ(first_k_map.at('S').size(), 23);  // как в вашем выводе
}
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "lrk_parser/config.hpp"
#include "lrk_parser/grammar.hpp"
#include "lrk_parser/rule.hpp"
#include "lrk_parser/first_k.hpp"

// --- ПРЕДПОЛАГАЕМЫЕ ИНКЛЮДЫ ВАШЕГО ПРОЕКТА ---
// #include "config.hpp"
// #include "rule.hpp"
// #include "grammar.hpp"
// #include "first_k.hpp"

// Использование пространств имен для удобства
using namespace lrk_parser;
using namespace lrk_parser::details;

// ------------------------------------------------------------------
// ACCESSOR ДЛЯ ДОСТУПА К ПРИВАТНЫМ МЕТОДАМ
// ------------------------------------------------------------------

// Макрос #ifdef UNIT_TESTS должен быть определен в вашем файле (или в этом).

// Класс-обёртка для доступа к приватным методам FirstK
class FirstKTestAccessor : public FirstK {
 public:
  // Используем ваш конструктор
  FirstKTestAccessor(const Grammar& grammar, const std::size_t& k)
      : FirstK(grammar, k) {}

  // 1. Публичный доступ к приватному статическому методу union_k_sets
  static void PublicUnionKSets(UsetT<StringT>& lhs_set,
                               const UsetT<StringT>& rhs_set) {
    FirstK::union_k_sets(lhs_set, rhs_set);
  }

  // 2. Публичный доступ к приватному методу concat_k_sets
  UsetT<StringT> PublicConcatKSets(const UsetT<StringT>& lhs_set,
                                   const UsetT<StringT>& rhs_set) const {
    // Внутри Accessor мы можем вызвать приватный метод, используя k_ из объекта
    return concat_k_sets(lhs_set, rhs_set);
  }

  // 3. Доступ к приватному полю first_k_ для проверки результатов
  const UmapT<CharT, UsetT<StringT>>& GetFirstKMap() const {
      return first_k_;
  }
};

// ------------------------------------------------------------------
// TEST FIXTURE (Использует ваш Grammar::init_with_strs)
// ------------------------------------------------------------------

// Структура для инициализации тестовых данных и объекта Grammar
struct FirstKTestGrammar : public ::testing::Test {
  // Параметры для Grammar::init_with_strs
  StringT T = "abc"; // Терминалы
  StringT N = "SAB"; // Нетерминалы (хотя они не используются в init_with_strs в mock)
  CharT StartSym = 'S';

  // Правила для грамматики K=1 (для тестов First1)
  VectorT<StringT> RulesStr_K1 = {"S->AB", "S->c", "A->a", "A->", "B->b", "B->"};

  // Правила для грамматики K=2 (для тестов First2)
  VectorT<StringT> RulesStr_K2 = {"S->Aa", "A->Bc", "B->d", "B->"};

  // Инициализация объекта Grammar через ваш фабричный метод
  Grammar create_grammar(const VectorT<StringT>& rules) {
      return Grammar::init_with_strs(T, N, rules, StartSym);
  }
};

// ------------------------------------------------------------------
// GOOGLE TESTS
// ------------------------------------------------------------------


TEST_F(FirstKTestGrammar, UnionKSets_Private) {
  UsetT<StringT> lhs = {"a", "b", "c"};
  const UsetT<StringT> rhs = {"b", "d", "e"};

  // Вызов приватного статического метода через Accessor
  FirstKTestAccessor::PublicUnionKSets(lhs, rhs);

  EXPECT_EQ(lhs.size(), 5);
  EXPECT_TRUE(lhs.count("a"));
  EXPECT_TRUE(lhs.count("b"));
  EXPECT_TRUE(lhs.count("c"));
  EXPECT_TRUE(lhs.count("d"));
  EXPECT_TRUE(lhs.count("e"));
}

TEST_F(FirstKTestGrammar, ConcatKSets_K2_Truncation) {
  // Создаем объект Accessor с k=2 (грамматика не важна, пока нет правил)
  Grammar g = create_grammar({});
  FirstKTestAccessor fk_acc(g, 2); 

  // Case 1: Concatenation with epsilon and truncation
  UsetT<StringT> set1 = {"ab", "a"};
  UsetT<StringT> set2 = {"c", ""}; // c и epsilon
  
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
  // Создаем объект Accessor с k=3
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
  // 1. Инициализация (используя ваш Grammar::init_with_strs)
  Grammar g = create_grammar(RulesStr_K1);
  FirstKTestAccessor fk_acc(g, 1); // k = 1

  const auto& first_k_map = fk_acc.GetFirstKMap();

  // 2. Проверка терминалов
  EXPECT_EQ(first_k_map.at('a'), (UsetT<StringT>{"a"}));
  EXPECT_EQ(first_k_map.at('b'), (UsetT<StringT>{"b"}));
  EXPECT_EQ(first_k_map.at('c'), (UsetT<StringT>{"c"}));

  // 3. Проверка нетерминалов (First1)
  
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
  FirstKTestAccessor fk_acc(g, 2); // k = 2

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
  EXPECT_TRUE(first_k_map.at('S').count("dc")); // 'dca' усекается до 'dc'
  EXPECT_TRUE(first_k_map.at('S').count("ca"));
}

TEST_F(FirstKTestGrammar, ComputeFirstKForString_K3) {
  // Грамматика, где: First3(A) = {a, c, ""}, First3(B) = {c, ""}
  VectorT<StringT> rules = {
      {'A', "a"},
      {'A', "Bc"},
      {'B', "c"},
      {'B', ""},
  };
  Grammar g = create_grammar(rules);
  FirstK fk(g, 3); // k = 3
  
  // Проверяемая строка: "ABa"
  // First3("ABa") = First3(A) * First3(B) * First3(a)
  // Ожидаемый результат: {aca, acc, a, cca, cc, ca}
  
  UsetT<StringT> expected = {"aca", "acc", "a", "cca", "cc", "ca"};
  UsetT<StringT> result = fk.compute_first_k("ABa");

  EXPECT_EQ(result.size(), 6);
  EXPECT_EQ(result, expected);
}
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "lrk_parser/action_table.hpp"
#include "lrk_parser/canonical_collection.hpp"
#include "lrk_parser/config.hpp"
#include "lrk_parser/first_k.hpp"
#include "lrk_parser/grammar.hpp"

// Подключаем класс-аксессор для тестов (если он в отдельном файле)
// Или определяем простую базу, если accessor не нужен для ActionTable 
// (так как методы доступа публичные).
// Мы используем базу из предыдущего шага.

using namespace lrk_parser;
using namespace lrk_parser::details;

// Базовая фикстура (можно вынести в общий хедер тестов)
class ActionTableTest : public ::testing::Test {
 protected:
  Grammar grammar_;
  std::unique_ptr<FirstK> first_k_;
  std::unique_ptr<CanonicalCollection> canonical_collection_;

  // Хелпер для создания простой грамматики
  // S' -> S (0)
  // S -> A  (1)
  // A -> a  (2)
  void SetUpSimpleGrammar() {
    StringT T = "a";
    StringT N = "SA";
    CharT Start = 'S';
    VectorT<StringT> rules = {"S->A", "A->a"};
    grammar_ = Grammar::init_with_strs(T, N, rules, Start);
    first_k_ = std::make_unique<FirstK>(grammar_, 1);
    canonical_collection_ =
        std::make_unique<CanonicalCollection>(grammar_, *first_k_);
  }

  // Хелпер для создания грамматики с конфликтом Shift/Reduce (при k=1)
  // S -> A
  // S -> x b
  // A -> x
  // Строка "xb":
  // State: [S -> . x b], [A -> . x], Lookahead 'b'
  // Shift 'x' -> State N
  // State N: [S -> x . b] (Shift 'b'), [A -> x .] (Reduce A->x, Lookahead 'b')
  void SetUpConflictGrammar() {
    StringT T = "xb";
    StringT N = "SA";
    CharT Start = 'S';
    VectorT<StringT> rules = {"S->A", "S->xb", "A->x"};
    grammar_ = Grammar::init_with_strs(T, N, rules, Start);
    first_k_ = std::make_unique<FirstK>(grammar_, 1);
    canonical_collection_ =
        std::make_unique<CanonicalCollection>(grammar_, *first_k_);
  }
};

// Тест 1: Проверка базовых действий (Shift, Reduce, Accept)
// Грамматика: S->A, A->a
TEST_F(ActionTableTest, BasicShiftReduceAccept) {
  SetUpSimpleGrammar();

  // Строим таблицу действий
  ActionTable action_table(grammar_, *first_k_, *canonical_collection_);

  // Нам нужно найти ID состояний, чтобы проверить переходы.
  // В CanonicalCollection для этой грамматики (обычно):
  // I0: S'->.S, S->.A, A->.a
  // I_target_a: A->a. (после перехода по 'a')
  // I_target_A: S->A. (после перехода по 'A')
  // I_target_S: S'->S. (после перехода по 'S')

  // 1. Проверяем Shift action для 'a' из начального состояния (I0)
  // Ищем состояние, из которого есть переход по 'a'
  const auto& goto_table = canonical_collection_->get_goto_table();
  StateIdT state_0 = 0; // Обычно начальное
  
  // Ключ: State 0, Lookahead "a" (так как k=1)
  ActionKey shift_key{.state_id = state_0, .lookahead = "a"};

  ASSERT_TRUE(action_table.has_parse_action(shift_key));
  Action shift_act = action_table.get_parse_action(shift_key);
  
  EXPECT_EQ(shift_act.type, ActionType::Shift);
  // Значение должно быть ID следующего состояния
  TransitionKey goto_key{.current_state_id = state_0, .symbol = 'a'};
  ASSERT_TRUE(goto_table.contains(goto_key));
  EXPECT_EQ(shift_act.value, goto_table.at(goto_key));

  // 2. Проверяем Reduce action для A -> a (Rule 2)
  // Это происходит в состоянии, куда мы попали после Shift 'a'
  StateIdT state_after_a = goto_table.at(goto_key);
  
  // Lookahead: для S->A, A->a и k=1, Lookahead у A->a будет 'eof' (пустая строка)
  // так как S - это конец.
  ActionKey reduce_key{.state_id = state_after_a, .lookahead = ""};
  
  ASSERT_TRUE(action_table.has_parse_action(reduce_key));
  Action reduce_act = action_table.get_parse_action(reduce_key);
  
  EXPECT_EQ(reduce_act.type, ActionType::Reduce);
  EXPECT_EQ(reduce_act.value, 2); // Индекс правила A -> a

  // 3. Проверяем Accept action
  // Это происходит в состоянии после перехода по 'S' (S' -> S .)
  TransitionKey goto_S_key{.current_state_id = state_0, .symbol = 'S'};
  ASSERT_TRUE(goto_table.contains(goto_S_key));
  StateIdT state_after_S = goto_table.at(goto_S_key);

  ActionKey accept_key{.state_id = state_after_S, .lookahead = ""};
  
  ASSERT_TRUE(action_table.has_parse_action(accept_key));
  Action accept_act = action_table.get_parse_action(accept_key);
  
  EXPECT_EQ(accept_act.type, ActionType::Accept);
  EXPECT_EQ(accept_act.value, 0); // Значение для Accept обычно 0 или игнорируется
}

// Тест 2: Обнаружение конфликтов (Shift/Reduce)
TEST_F(ActionTableTest, DetectsShiftReduceConflict) {
  SetUpConflictGrammar();
  
  // CanonicalCollection построится успешно (он просто строит граф).
  // Ошибка должна вылететь при попытке заполнить таблицу действий,
  // когда в одну ячейку пытаются записать и Shift и Reduce.
  
  EXPECT_THROW({
    ActionTable action_table(grammar_, *first_k_, *canonical_collection_);
  }, std::runtime_error);
}

// Тест 3: Копирование и Перемещение
TEST_F(ActionTableTest, CopyAndMove) {
  SetUpSimpleGrammar();
  ActionTable src_table(grammar_, *first_k_, *canonical_collection_);
  
  // Copy
  ActionTable copy_table = src_table;
  ActionKey key{.state_id = 0, .lookahead = "a"};
  EXPECT_TRUE(copy_table.has_parse_action(key));
  
  // Move
  ActionTable move_table = std::move(src_table);
  EXPECT_TRUE(move_table.has_parse_action(key));
}
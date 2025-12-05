#include <gtest/gtest.h>

#include <string>
#include <vector>

// Подключаем ваши заголовки
#include "lrk_parser/canonical_collection.hpp"  // Предлагаемое имя для вашего файла
#include "lrk_parser/config.hpp"
#include "lrk_parser/first_k.hpp"
#include "lrk_parser/grammar.hpp"
#include "lrk_parser/situation.hpp"

namespace lrk_parser::details {

// Класс-аксессор для доступа к приватным методам
class CanonicalCollectionTestAccessor : public CanonicalCollection {
 public:
  // Мы не можем наследовать конструктор напрямую, так как базовый удален или
  // специфичен, но нам нужно создать объект. Так как конструктор вызывает
  // build_goto_table, мы просто используем стандартный конструктор.
  CanonicalCollectionTestAccessor(const Grammar& grammar, const FirstK& fk)
      : CanonicalCollection(grammar, fk) {}

  // Обертки над приватными методами

  details::Situations PublicClosure(const Grammar& grammar,
                                    const FirstK& fist_k,
                                    details::Situations kernel_set) const {
    return closure(grammar, fist_k, std::move(kernel_set));
  }

  details::Situations PublicComputeGo(const Grammar& grammar,
                                      const FirstK& fist_k, std::size_t I_idx,
                                      CharT X) const {
    return compute_go_situation(grammar, fist_k, I_idx, X);
  }

  std::pair<StateIdT, bool> PublicInsertSituations(details::Situations I) {
    return insert_sutiations(std::move(I));
  }

  // Доступ к полям для проверки внутреннего состояния
  const StatesT& GetStatesInternal() const { return states_; }
};

}  // namespace lrk_parser::details

using namespace lrk_parser;
using namespace lrk_parser::details;

// Фикстура для настройки Грамматики и FirstK
class CanonicalCollectionTest : public ::testing::Test {
 protected:
  // Грамматика 1: Простая для тестов Closure
  // S -> A
  // A -> a
  void SetUpSimpleGrammar() {
    StringT T = "a";
    StringT N = "SA";
    CharT Start = 'S';
    VectorT<StringT> rules = {"S->A", "A->a"};
    grammar_ = Grammar::init_with_strs(T, N, rules, Start);
    // k=1 достаточно для базовых тестов
    first_k_ = std::make_unique<FirstK>(grammar_, 1);
  }

  // Грамматика 2: Рекурсивная для тестов GOTO и Closure с лукахедом
  // S -> C C
  // C -> c C
  // C -> d
  void SetUpRecursiveGrammar() {
    StringT T = "cd";
    StringT N = "SC";
    CharT Start = 'S';
    VectorT<StringT> rules = {"S->CC", "C->cC", "C->d"};
    grammar_ = Grammar::init_with_strs(T, N, rules, Start);
    first_k_ = std::make_unique<FirstK>(grammar_, 1);
  }

  Grammar grammar_;
  std::unique_ptr<FirstK> first_k_;
};

// Тест 1: Проверка метода Closure (Замыкание)
// Покрывает логику расширения ситуаций.
TEST_F(CanonicalCollectionTest, ClosureLogic) {
  SetUpSimpleGrammar();  // S'->S, S->A, A->a. Индексы: 0, 1, 2
                         // (предположительно)

  // Получаем индекс правила S -> A. Предполагаем, что S->A это второе правило
  // (индекс 1). ЛУЧШЕ: Получить его динамически, если Grammar это позволяет.
  // Если нет, используем хардкод, но с осознанием порядка правил:
  const std::size_t S_A_RuleIdx = 1;  // Assuming S->A is index 1

  CanonicalCollectionTestAccessor collection(grammar_, *first_k_);

  // Создаем ядро ситуации: [S -> . A, ""] (Индекс 1)
  Situations kernel;
  kernel.emplace(
      Situation{.rule_idx = S_A_RuleIdx, .dot_pose = 0, .actpref = ""});

  // Вызываем closure
  Situations result = collection.PublicClosure(grammar_, *first_k_, kernel);

  // Ожидаем: [S -> . A, ""] (исходная) + [A -> . a, ""]
  EXPECT_EQ(result.size(), 2) << "Closure for [S -> . A] must be 2 situations: "
                                 "the kernel and [A -> . a].";

  // Проверяем наличие A -> . a
  bool found_A_rule = false;
  for (const auto& sit : result) {
    // Получаем левую часть правила по индексу
    const auto& rule = grammar_.get_rule_by_idx(sit.rule_idx);
    if (rule.lhs == 'A' && sit.dot_pose == 0) {
      found_A_rule = true;
    }
  }
  EXPECT_TRUE(found_A_rule)
      << "Closure should add production for non-terminal A";
}

// Тест 2: Ветвления в Closure (Terminal vs Non-Terminal vs End of Rule)
TEST_F(CanonicalCollectionTest, ClosureBranchesEdges) {
  SetUpSimpleGrammar();  // S'->S (0), S->A (1), A->a (2)
  CanonicalCollectionTestAccessor collection(grammar_, *first_k_);

  // Использовать индекс 2 для правила A -> a
  const std::size_t A_A_RuleIdx = 2;

  // Случай А: Точка перед Терминалом [A -> . a, ""]
  Situations kernel;
  kernel.emplace(
      Situation{.rule_idx = A_A_RuleIdx, .dot_pose = 0, .actpref = ""});

  Situations res1 = collection.PublicClosure(grammar_, *first_k_, kernel);
  // После точки стоит терминал 'a', замыкание не должно добавлять ничего
  // (размер 1).
  EXPECT_EQ(res1.size(), 1) << "Test A: Closure on [A -> . a] should only "
                               "contain the kernel (1 situation).";

  // Случай Б: Точка в конце правила [A -> a ., ""]
  Situations kernel_end;
  kernel_end.emplace(
      Situation{.rule_idx = A_A_RuleIdx, .dot_pose = 1, .actpref = ""});

  Situations res2 = collection.PublicClosure(grammar_, *first_k_, kernel_end);
  // Точка в конце (dot_pose >= rhs.size()), замыкание не должно добавлять
  // ничего (размер 1).
  EXPECT_EQ(res2.size(), 1) << "Test B: Closure on [A -> a .] should only "
                               "contain the kernel (1 situation).";
}

TEST_F(CanonicalCollectionTest, ComputeGoLogic) {
  SetUpSimpleGrammar();  // S'->S (0), S->A (1), A->a (2)
  CanonicalCollectionTestAccessor collection(grammar_, *first_k_);

  const StateIdT I0_id = 0;
  const std::size_t S_A_RuleIdx = 1;  // Правильный индекс для S -> A

  // 1. Проверяем, что состояние I0_id существует и не пусто
  ASSERT_LT(I0_id, collection.get_states().size());
  ASSERT_FALSE(collection.get_states()[I0_id].empty());

  // 2. Переход по символу 'A'
  // Используем ID состояния, а не множество Situations
  Situations next_I =
      collection.PublicComputeGo(grammar_, *first_k_, I0_id, 'A');

  // Ожидаем: [S -> A ., ""]
  EXPECT_EQ(next_I.size(), 1);
  const auto& sit = *next_I.begin();

  // Проверяем, что это корректная ситуация S -> A .
  EXPECT_EQ(sit.rule_idx, S_A_RuleIdx)
      << "Rule index must be for S -> A (index 1).";
  EXPECT_EQ(sit.dot_pose, 1);
  EXPECT_EQ(sit.actpref, "");

  // 3. Переход по неверному символу (например, 'b')
  Situations empty_I =
      collection.PublicComputeGo(grammar_, *first_k_, I0_id, 'b');
  EXPECT_TRUE(empty_I.empty());
}
// Тест 4: Insert Situations (Управление ID состояний)
TEST_F(CanonicalCollectionTest, InsertSituations) {
  SetUpSimpleGrammar();
  CanonicalCollectionTestAccessor collection(grammar_, *first_k_);

  Situations s1;
  s1.emplace(Situation{.rule_idx = 0, .dot_pose = 0, .actpref = "a"});

  // Первая вставка
  auto [id1, inserted1] = collection.PublicInsertSituations(s1);
  // ВАЖНО: Конструктор CanonicalCollection уже заполнил states_ начальными
  // состояниями. id1 будет новым ID.
  EXPECT_TRUE(inserted1);

  // Повторная вставка того же множества
  auto [id2, inserted2] = collection.PublicInsertSituations(s1);
  EXPECT_FALSE(inserted2);  // Не должно вставиться
  EXPECT_EQ(id1, id2);      // ID должен совпасть

  // Проверка целостности вектора states_
  // Размер вектора должен соответствовать последнему выданному ID + 1 (с учетом
  // того, что конструктор уже что-то добавил)
  EXPECT_EQ(collection.GetStatesInternal().size() - 1, id1);
}

// Тест 5: Полный цикл построения (Integration via Constructor)
TEST_F(CanonicalCollectionTest, FullBuildRecursiveGrammar) {
  SetUpRecursiveGrammar();  // S->CC, C->cC, C->d

  // Создаем объект штатным образом
  CanonicalCollection collection(grammar_, *first_k_);

  // Проверяем таблицу GOTO
  const auto& goto_table = collection.get_goto_table();
  EXPECT_FALSE(goto_table.empty());

  const auto& states = collection.get_states();

  // 1. Находим индекс правила S' -> S (Grammar::kStarSym = '@')
  const auto& s_prime_rules = grammar_.get_rules_idxs(Grammar::kStarSym);
  ASSERT_EQ(s_prime_rules.size(), 1)
      << "Augmented grammar must have exactly one S' rule.";
  auto s_prime_rule_idx = *s_prime_rules.begin();

  // 2. Ищем переход GOTO(I0, S)
  bool found_start_trans = false;
  StateIdT next_state_id = 0;

  for (const auto& [key, next_state] : goto_table) {
    // Проверяем переход по стартовому символу S из начального состояния (ID 0)
    if (key.symbol == 'S' && key.current_state_id == 0) {
      found_start_trans = true;
      next_state_id = next_state;
      break;
    }
  }
  EXPECT_TRUE(found_start_trans)
      << "Should have transition by Start symbol 'S' from Initial state I0";

  // 3. Детальная проверка целевого состояния (I1)
  if (found_start_trans) {
    const auto& target_sits = states[next_state_id];

    // Ожидаемое состояние I1: [S' -> S ., ""]
    // (Свертка S' -> S, Lookahead = "")
    EXPECT_EQ(target_sits.size(), 1)
        << "State after GOTO(I0, S) should contain exactly one situation: the "
           "accept situation.";

    const auto& sit = *target_sits.begin();

    EXPECT_EQ(sit.rule_idx, s_prime_rule_idx)
        << "Rule index must be for S' -> S.";
    EXPECT_EQ(sit.dot_pose, 1)
        << "Dot must be at the end of the rule (dot_pose = 1).";
    EXPECT_EQ(sit.actpref, "")
        << "Accept situation lookahead must be empty string (EOF marker).";
  }
}

// Тест 6: Getters и Move семантика
TEST_F(CanonicalCollectionTest, GettersAndMove) {
  SetUpSimpleGrammar();
  CanonicalCollection collection(grammar_, *first_k_);

  // Проверка const getters
  EXPECT_NO_THROW(std::ignore = collection.get_states());
  EXPECT_NO_THROW(std::ignore = collection.get_state_to_id_map());
  EXPECT_FALSE(collection.get_goto_table().empty());

  // Проверка take_goto_table (перемещение)
  auto table_copy = collection.get_goto_table();    // копия
  auto table_moved = collection.take_goto_table();  // move

  EXPECT_EQ(table_copy, table_moved);
  // После take_goto_table внутреннее состояние goto_table_ неопределено или
  // пусто, но метод возвращает копию, если поле не было перемещено ранее. В
  // вашей реализации: return goto_table_; (возврат по значению, создаст копию
  // или мув).

  // Проверка на присваивание/копирование (так как они default)
  CanonicalCollection collection2 = collection;  // Copy constructor
  EXPECT_EQ(collection2.get_states().size(), collection.get_states().size());
}

// Тест 7: Краевой случай пустого перехода (ветка if (next_sits.empty())
// return;) Этот кейс покрывает лямбду process_symbol_transition внутри
// build_goto_table
TEST_F(CanonicalCollectionTest, NoTransitionEdgeCase) {
  // S -> a
  // S -> b
  StringT T = "ab";
  StringT N = "S";
  CharT Start = 'S';
  VectorT<StringT> rules = {"S->a", "S->b"};
  Grammar g = Grammar::init_with_strs(T, N, rules, Start);
  FirstK fk(g, 1);

  CanonicalCollection collection(g, fk);

  // В состоянии, где мы считали 'a' (S -> a .), переходов быть не должно.
  // build_goto_table перебирает ВСЕ терминалы для каждого состояния.
  // Значит, для состояния (S -> a .) он вызовет compute_go_situation с символом
  // 'b'. compute_go вернет пустое множество. Ветка if (next_sits.empty()) {
  // return; } сработает. Это неявная проверка (покрытие кода), так как мы не
  // можем легко проверить "return" извне, но сам факт успешного построения без
  // крэшей и лишних переходов говорит о корректности.

  const auto& table = collection.get_goto_table();
  // Убедимся, что нет переходов из завершенных состояний
  // (это косвенно подтверждает работу проверки на empty)
  for (const auto& [key, val] : table) {
    // Проверяем состояние назначения
    const auto& dest_sits = collection.get_states()[val];
    // Если состояние финальное (точка в конце), из него не должно быть выходов
    // в таблице
    bool is_final = true;
    for (const auto& s : dest_sits) {
      const auto& rhs = g.get_rule_by_idx(s.rule_idx).rhs;
      if (s.dot_pose < rhs.size()) is_final = false;
    }

    if (is_final) {
      // Проверяем, есть ли этот val как current_state_id в ключах таблицы
      for (const auto& [k2, v2] : table) {
        EXPECT_NE(k2.current_state_id, val)
            << "Final state should not have outgoing transitions";
      }
    }
  }
}
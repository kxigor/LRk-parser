## LR(k) Parser Implementation

Этот проект представляет собой реализацию LR(k) синтаксического анализатора (парсера) на C++.

## Обзор Возможностей
  * **LR(k) Анализатор:** Ядро проекта — это реализация LR(k) парсера, способного анализировать входные строки, используя заданную грамматику и $\text{k}$-просмотр (lookahead).
  * **Качество Кода:** Используется строгий контроль качества с помощью инструментов статического анализа и форматирования.
  * **Система Сборки CMake:** Использование `CMakePresets.json` для удобной настройки различных конфигураций сборки (Development, CI, Sanitizers, Coverage).

-----

## Сборка и Запуск

### Требования

Для сборки нужны CMake, компилятор C++23 и Ninja. Остальные инструменты используются соответствующими проверками:

| Инструмент | Версия | Назначение |
| :--- | :--- | :--- |
| **`CMake`** | 3.25+ | Система сборки |
| **`C++`** | GCC 16, Clang 22 (проверено) | Компиляция библиотеки с C++23 |
| **`Ninja`** | | Генератор сборки |
| **`Google Test`** | | Модульное тестирование |
| **`clang-format`** | 22 (проверено) | Проверка форматирования |
| **`clang-tidy`** | 22 (проверено) | Статический анализ |
| **`lcov` / `genhtml` / `gcov`** | | Отчёт о покрытии |

### Шаги Сборки

1. Конфигурирование (например, отладочная сборка с Address Sanitizer):

```bash
cmake --preset dev-debug-asan
```

2. Сборка:

```bash
cmake --build --preset dev-debug-asan
```

Для сборки только с UBSan замените имя пресета на `dev-debug-ubsan`.
Пресет `dev-debug-msan` использует Clang и требует инструментированные сборки стандартной библиотеки и Google Test; с обычными системными библиотеками его тесты дают ложные срабатывания.

### Тестирование и Покрытие Кода

Все модульные тесты запускаются с помощью `ctest`.

#### Запуск Тестов

```bash
ctest --preset dev-debug-asan
```

При наличии Google Test, clang-format и clang-tidy пресет запускает модульные тесты и обе статические проверки. Отсутствующий инструмент отключает только связанную с ним проверку. Для UBSan используйте `ctest --preset dev-debug-ubsan`.

#### Покрытие Кода (Coverage)

Для генерации отчета о покрытии:

1. Сконфигурируйте и соберите проект с пресетом `dev-debug-coverage`:

```bash
cmake --preset dev-debug-coverage
cmake --build --preset dev-debug-coverage
```

2. Запустите цель `coverage`: она выполнит тесты и создаст отчёт через `lcov` и `genhtml`.

```bash
cmake --build --preset dev-debug-coverage --target coverage
```

3. Откройте отчёт по исходникам библиотеки (`src` и `include`):

HTML-отчет будет сохранен в папке сборки.
```bash
build/dev-debug-coverage/coverage_report/index.html
```

-----

## Использование Библиотеки

Основной интерфейс — `lrk_parser::Parser` из `lrk_parser/parser.hpp`. Построение грамматики объявлено в `lrk_parser/grammar.hpp`, текстовый адаптер — в `lrk_parser/text_grammar.hpp`.

Для подключения из другого CMake-проекта:

```cmake
add_subdirectory(external/LRk-parser)
target_link_libraries(my_app PRIVATE lrk_parser::lrk_parser)
```

Цель CMake передаёт потребителю требование C++23. При подключении через `add_subdirectory` приложение и тесты библиотеки не собираются. При самостоятельной сборке ими управляют независимые опции `LRK_PARSER_BUILD_APP` и `ENABLE_TESTING`.

### Основной Интерфейс

```cpp
static std::expected<Parser, CompileError> Compile(const Grammar& grammar,
                                                   std::size_t k);
bool Accepts(StringViewT word) const;
std::size_t Lookahead() const;
```

Значение `k` задаёт вызывающий код; при необходимости он сам организует перебор.

Грамматика создаётся через `MakeGrammar(GrammarSpec)` или текстовый адаптер `ParseGrammar`:

```cpp
#include "lrk_parser/parser.hpp"
#include "lrk_parser/text_grammar.hpp"

int main() {
  const auto grammar = lrk_parser::ParseGrammar(
      "ab", "S", {"S->aSb", "S->"}, 'S');
  if (!grammar) {
    return 1;
  }

  auto parser = lrk_parser::Parser::Compile(*grammar, 1);
  if (!parser) {
    return 1;
  }
  return parser->Accepts("aabb") ? 0 : 1;
}
```

Структурированный вариант той же грамматики:

```cpp
auto grammar = lrk_parser::MakeGrammar({
    .terminals = "ab",
    .nonterminals = "S",
    .rules = {{'S', "aSb"}, {'S', ""}},
    .start = 'S',
});
```

Обе фабрики возвращают `std::expected`. `GrammarError` содержит причину, символ и, если ошибка относится к правилу, его индекс с нуля. У `ParseGrammar` ошибка — `std::variant<RuleSyntaxError, GrammarError>`; синтаксическая ошибка указывает индекс строки с неверной записью `S->rhs`.

`Parser::Compile` создаёт готовый парсер или возвращает `CompileError`: недопустимый `k=0` либо конфликт действий. Готовый парсер владеет данными для распознавания и не зависит от времени жизни `Grammar`.

`lrk_parser/output.hpp` добавляет вывод грамматики, парсера и ошибок через поток или `std::format`:

```cpp
#include <format>
#include <iostream>
#include "lrk_parser/output.hpp"

std::cout << *parser;
const auto description = std::format("{}", *parser);
```

Внутренние стадии FIRST, каноническая коллекция и таблицы находятся в `lrk_parser/details/` и могут исследоваться отдельно от готового парсера.

`Grammar` владеет данными и предоставляет доступ только для чтения. Проверяются алфавиты, стартовый символ и правила, включая наличие продукций у используемых нетерминалов. Порядок и дубликаты правил сохраняются. Служебное стартовое правило добавляется только во внутреннее представление парсера.

Тип символа задаётся `CharT` (сейчас `char`); строки — `StringT`/`StringViewT`. Текущая реализация работает с байтами, включая `@` и `\0`; ε задаётся пустой правой частью. `MakeGrammar` сохраняет пробелы в правилах, поэтому значимые пробелы нужно объявить терминалами. `ParseGrammar` удаляет из строк правил ASCII-пробелы, табуляцию и переводы строк (` \t\n\r\f\v`).

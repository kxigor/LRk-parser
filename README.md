# LR(k) parser

C++23 library for building LR(k) parsers and recognizing byte strings. The caller chooses `k` (at least 1).

## Build and test

Requires CMake 3.25+, Ninja, and a C++23 compiler (tested with GCC 16 and Clang 22).

```sh
cmake --preset dev-debug-asan
cmake --build --preset dev-debug-asan
ctest --preset dev-debug-asan
```

Google Test enables unit tests; clang-format and clang-tidy enable their CTest checks. Other presets include `dev-debug-ubsan`, `dev-debug-tsan`, and `ci-release`. The `dev-debug-msan` preset requires an instrumented C++ standard library and Google Test.

For an HTML coverage report, install `gcov`, `lcov`, and `genhtml`, then run:

```sh
cmake --preset dev-debug-coverage
cmake --build --preset dev-debug-coverage --target coverage
```

Report: `build/dev-debug-coverage/coverage_report/index.html`.

## Use in another CMake project

```cmake
add_subdirectory(external/LRk-parser)
target_link_libraries(my_app PRIVATE lrk_parser::lrk_parser)
```

The target requires C++23. When included as a subproject, the example app and tests are not built.

## API

```cpp
#include "lrk_parser/grammar.hpp"
#include "lrk_parser/parser.hpp"

int main() {
  auto grammar = lrk_parser::Grammar::FromTextRules(
      "ab", "S", {"S->aSb", "S->"}, 'S');
  if (!grammar) {
    return 1;
  }

  auto parser = lrk_parser::Parser::Compile(*grammar, 1);
  return parser && parser->Accepts("aabb") ? 0 : 1;
}
```

`Grammar::FromSpec(GrammarSpec)` accepts structured rules. `FromTextRules` parses `S->rhs` strings and removes ASCII whitespace from them; `FromSpec` preserves whitespace. An empty right-hand side means ε.

Both grammar factories and `Parser::Compile` return `std::expected`. Include `lrk_parser/output.hpp` for stream and `std::format` output.

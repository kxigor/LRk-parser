include_guard()

find_program(CLANG_TIDY_EXE
  NAMES clang-tidy-22 clang-tidy-21 clang-tidy-20 clang-tidy-19 clang-tidy
  DOC "Clang-Tidy executable"
)

find_package(Python3 QUIET COMPONENTS Interpreter)

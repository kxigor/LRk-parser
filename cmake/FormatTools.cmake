include_guard()

find_program(CLANG_FORMAT_EXE
  NAMES clang-format-22 clang-format-21 clang-format-20 clang-format-19 clang-format
  DOC "Clang-Format executable"
)

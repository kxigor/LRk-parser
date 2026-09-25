file(GLOB_RECURSE FORMAT_FILES CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/app/*.cpp"
  "${PROJECT_SOURCE_DIR}/app/*.hpp"
  "${PROJECT_SOURCE_DIR}/src/*.cpp"
  "${PROJECT_SOURCE_DIR}/src/*.hpp"
  "${PROJECT_SOURCE_DIR}/include/*.cpp"
  "${PROJECT_SOURCE_DIR}/include/*.hpp"
  "${PROJECT_SOURCE_DIR}/tests/unit/*.cpp"
  "${PROJECT_SOURCE_DIR}/tests/unit/*.hpp"
)

file(GLOB_RECURSE TIDY_FILES CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/src/*.cpp"
)
if(LRK_PARSER_BUILD_APP)
  file(GLOB_RECURSE APP_TIDY_FILES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/app/*.cpp"
  )
  list(APPEND TIDY_FILES ${APP_TIDY_FILES})
endif()
if(ENABLE_UNITTESTS AND GTest_FOUND)
  file(GLOB_RECURSE UNIT_TIDY_FILES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/tests/unit/*.cpp"
  )
  list(APPEND TIDY_FILES ${UNIT_TIDY_FILES})
endif()

if(ENABLE_FORMAT_CHECK AND CLANG_FORMAT_EXE)
  add_test(NAME FormatCheck
    COMMAND bash "${PROJECT_SOURCE_DIR}/tests/scripts/check_format.sh"
      --format-exec "${CLANG_FORMAT_EXE}"
      --config "${PROJECT_SOURCE_DIR}/.clang-format"
      --files ${FORMAT_FILES}
  )
endif()

if(ENABLE_TIDY_CHECK AND CLANG_TIDY_EXE AND Python3_Interpreter_FOUND)
  add_test(NAME TidyCheck
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/scripts/check_tidy.py"
      --tidy-exec "${CLANG_TIDY_EXE}"
      --build-dir "${CMAKE_BINARY_DIR}"
      --config "${PROJECT_SOURCE_DIR}/.clang-tidy"
      ${TIDY_FILES}
  )
endif()

include_guard()

add_library(project_coverage INTERFACE)

if(ENABLE_COVERAGE)
  target_compile_options(project_coverage INTERFACE
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:--coverage>
  )
  target_link_options(project_coverage INTERFACE
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:--coverage>
  )

  find_program(LCOV_EXECUTABLE NAMES lcov)
  find_program(GENHTML_EXECUTABLE NAMES genhtml)

  if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
    set(COVERAGE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/coverage_report")
    set(COVERAGE_INFO_FILE "${COVERAGE_OUTPUT_DIR}/coverage.info")

    add_custom_target(coverage
      COMMAND ${CMAKE_COMMAND} -E make_directory "${COVERAGE_OUTPUT_DIR}"
      COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}"
      COMMAND ${LCOV_EXECUTABLE} --zerocounters --directory "${CMAKE_BINARY_DIR}"
      COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CMAKE_BINARY_DIR}" --output-on-failure
      COMMAND ${LCOV_EXECUTABLE} --capture --directory "${CMAKE_BINARY_DIR}/src" --output-file "${COVERAGE_INFO_FILE}"
      COMMAND ${LCOV_EXECUTABLE} --extract "${COVERAGE_INFO_FILE}"
        "${PROJECT_SOURCE_DIR}/src/*" "${PROJECT_SOURCE_DIR}/include/*"
        --output-file "${COVERAGE_INFO_FILE}"
      COMMAND ${GENHTML_EXECUTABLE} "${COVERAGE_INFO_FILE}" --output-directory "${COVERAGE_OUTPUT_DIR}"
      COMMENT "Generating coverage report in ${COVERAGE_OUTPUT_DIR}"
      USES_TERMINAL
      VERBATIM
    )
  else()
    message(WARNING "LCOV/GenHTML tools not found. Coverage target skipped.")
  endif()
endif()

cmake_minimum_required(VERSION 3.25)

project(lrk-parser LANGUAGES CXX)

if(PROJECT_IS_TOP_LEVEL)
  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
  endif()

  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
  set(CMAKE_CXX_EXTENSIONS OFF)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

  option(LRK_PARSER_BUILD_APP "Build the example application." ON)
  option(ENABLE_TESTING "Enable CTest checks." ON)
  option(ENABLE_UNITTESTS "Enable Google Test unit tests." ON)
  option(ENABLE_FORMAT_CHECK "Register the Clang-Format check." ON)
  option(ENABLE_TIDY_CHECK "Register the Clang-Tidy check." ON)
  option(ENABLE_COVERAGE "Enable code coverage generation." OFF)

  include(ProjectOptions)
  include(ProjectSanitizers)
  include(ProjectCoverage)
endif()

add_subdirectory(src)

if(PROJECT_IS_TOP_LEVEL)
  if(LRK_PARSER_BUILD_APP)
    add_subdirectory(app)
  endif()

  if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
  endif()
endif()

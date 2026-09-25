add_library(lrk_parser
  grammar.cpp
  text_grammar.cpp
  prepared_grammar.cpp
  first_k.cpp
  canonical_collection.cpp
  goto_table.cpp
  action_table.cpp
  output/grammar.cpp
  output/automaton.cpp
  output/tables.cpp
  output/parser.cpp
  output/errors.cpp
  parser.cpp
)

add_library(lrk_parser::lrk_parser ALIAS lrk_parser)

target_compile_features(lrk_parser PUBLIC cxx_std_23)
set_target_properties(lrk_parser PROPERTIES CXX_EXTENSIONS OFF)

if(PROJECT_IS_TOP_LEVEL)
  target_link_libraries(lrk_parser
    PRIVATE
      project_options
      project_sanitizers
      project_coverage
  )
endif()

target_include_directories(lrk_parser
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

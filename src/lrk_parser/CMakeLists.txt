add_library(lrk_parser 
  grammar.cpp
  text_grammar.cpp
  prepared_grammar.cpp
  first_k.cpp
  canonical_collection.cpp
  goto_table.cpp
  action_table.cpp
  output_helpers.cpp
  parser.cpp
)

target_link_libraries(lrk_parser
  PRIVATE
    project_options
    project_sanitizers
    project_coverage
)

target_include_directories(lrk_parser
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

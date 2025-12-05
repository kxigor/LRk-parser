add_library(lrk_parser 
  parser.cpp
  grammar.cpp
  first_k.cpp
  goto_table.cpp
  canonical_collection.cpp
  output_helpers.cpp
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
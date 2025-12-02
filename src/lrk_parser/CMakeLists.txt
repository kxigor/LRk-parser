add_library(lrk_parser 
  parser.cpp
  grammar.cpp
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
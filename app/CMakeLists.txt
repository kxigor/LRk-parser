add_executable(lrk_parser_app main.cpp)

target_link_libraries(lrk_parser_app
  PRIVATE
    project_options
    project_sanitizers
    project_coverage
    lrk_parser::lrk_parser
)

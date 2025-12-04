add_gtest(basic_grammar
  SOURCES basic_grammar.cpp
  LIBRARIES
    lrk_parser
    project_options
    project_sanitizers
    project_coverage
)
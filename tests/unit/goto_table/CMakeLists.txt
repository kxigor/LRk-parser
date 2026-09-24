add_gtest(basic_goto_table
  SOURCES basic_goto_table.cpp
  LIBRARIES
    lrk_parser
    project_options
    project_sanitizers
    project_coverage
)
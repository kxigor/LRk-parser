add_gtest(basic_action_table
  SOURCES basic_action_table.cpp
  LIBRARIES
    lrk_parser
    project_options
    project_sanitizers
    project_coverage
)
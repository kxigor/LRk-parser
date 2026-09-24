add_gtest(output
  SOURCES output.cpp
  LIBRARIES
    lrk_parser
    project_options
    project_sanitizers
    project_coverage
)

add_gtest(first_k
  SOURCES first_k.cpp
  LIBRARIES
    lrk_parser::lrk_parser
    project_options
    project_sanitizers
    project_coverage
)

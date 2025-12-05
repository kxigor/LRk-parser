add_gtest(basic_canonical_collection
  SOURCES basic_canonical_collection.cpp
  LIBRARIES
    lrk_parser
    project_options
    project_sanitizers
    project_coverage
)
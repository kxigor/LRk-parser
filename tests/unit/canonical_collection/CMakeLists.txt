add_gtest(canonical_collection
  SOURCES canonical_collection.cpp
  LIBRARIES
    lrk_parser::lrk_parser
    project_options
    project_sanitizers
    project_coverage
  PROPERTIES TIMEOUT 30
)

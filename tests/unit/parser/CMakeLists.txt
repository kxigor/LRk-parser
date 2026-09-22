add_gtest(basic_parser
  SOURCES basic_parser.cpp recognition.cpp
  LIBRARIES
    lrk_parser
    project_options
    project_sanitizers
    project_coverage
  PROPERTIES TIMEOUT 30
)

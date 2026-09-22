add_gtest(text_grammar
  SOURCES text_grammar.cpp
  LIBRARIES
    lrk_parser
    project_options
    project_sanitizers
    project_coverage
)

add_gtest(validated_grammar
  SOURCES validated_grammar.cpp
  LIBRARIES
    lrk_parser
    project_options
    project_sanitizers
    project_coverage
)

add_gtest(prepared_grammar
  SOURCES prepared_grammar.cpp
  LIBRARIES
    lrk_parser
    project_options
    project_sanitizers
    project_coverage
)

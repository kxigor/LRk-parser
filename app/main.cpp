#include <iostream>

#include "lrk_parser/parser.hpp"
#include "lrk_parser/text_grammar.hpp"

int main() {
  const auto grammar =
      lrk_parser::ParseGrammar("ab", "S", {"S->aSb", "S->"}, 'S');
  if (!grammar) {
    std::cerr << "Invalid grammar\n";
    return 1;
  }

  lrk_parser::LrkParser parser;
  parser.fit(*grammar, 1);

  std::cout << parser << '\n';
  std::cout << std::boolalpha << parser.predict("aaabbb") << '\n';
}

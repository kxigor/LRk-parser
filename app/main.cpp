#include <iostream>

#include "lrk_parser/grammar.hpp"
#include "lrk_parser/output.hpp"
#include "lrk_parser/parser.hpp"

int main() {
  const auto grammar =
      lrk_parser::Grammar::FromTextRules("ab", "S", {"S->aSb", "S->"}, 'S');
  if (!grammar) {
    std::cerr << grammar.error() << '\n';
    return 1;
  }

  auto parser = lrk_parser::Parser::Compile(*grammar, 1);
  if (!parser) {
    std::cerr << parser.error() << '\n';
    return 1;
  }

  std::cout << *parser << '\n';
  std::cout << std::boolalpha << parser->Accepts("aaabbb") << '\n';
}

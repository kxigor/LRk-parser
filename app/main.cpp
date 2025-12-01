#include <cassert>
#include <cstddef>
#include <iostream>
#include <limits>
#include <print>
#include <string>
#include <utility>
#include <vector>

#include "lrk_parser/parser.hpp"

using CharT = lrk_parser::CharT;
using StringT = lrk_parser::StringT;
using Grammar = lrk_parser::Grammar;

int main() {
  std::size_t n{};
  std::cin >> n;
  std::size_t e{};
  std::cin >> e;
  std::size_t p{};
  std::cin >> p;

  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  StringT non_terminals;
  StringT terminals;
  std::getline(std::cin, non_terminals);
  std::getline(std::cin, terminals);
  assert(non_terminals.size() == n);
  assert(terminals.size() == e);
  std::vector<StringT> rules(p);
  for (std::size_t i = 0; i < p; ++i) {
    std::getline(std::cin, rules[i]);
  }
  CharT start_sym{};
  std::cin >> start_sym;

  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  auto grammar =
      Grammar::init_with_strs(std::move(terminals), std::move(non_terminals),
                              std::move(rules), start_sym);

  lrk_parser::LrkParser parser;

  parser.fit(std::move(grammar), 1);

  std::size_t m{};
  std::cin >> m;

  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  for (std::size_t i = 0; i < m; ++i) {
    StringT input;
    std::getline(std::cin, input);
    std::println("{}", parser.predict(input) ? "YES" : "NO");
  }
}
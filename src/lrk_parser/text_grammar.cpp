#include <string>
#include <utility>

#include "lrk_parser/grammar.hpp"

namespace lrk_parser {

std::expected<Grammar, TextGrammarError> Grammar::FromTextRules(
    StringT terminals, StringT nonterminals, std::vector<StringT> rules,
    CharT start) {
  GrammarSpec spec{std::move(terminals), std::move(nonterminals), {}, start};
  spec.rules.reserve(rules.size());
  for (std::size_t index = 0; index < rules.size(); ++index) {
    auto& text = rules[index];
    std::erase_if(text, [](CharT symbol) {
      return StringViewT{" \t\n\r\f\v"}.contains(symbol);
    });
    if (text.size() < 3 || text.substr(1, 2) != "->") {
      return std::unexpected{RuleSyntaxError{index}};
    }
    spec.rules.push_back({text[0], text.substr(3)});
  }
  auto grammar = FromSpec(std::move(spec));
  if (!grammar) {
    return std::unexpected{grammar.error()};
  }
  return std::move(*grammar);
}

}  // namespace lrk_parser

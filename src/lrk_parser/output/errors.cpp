#include <ostream>
#include <utility>
#include <variant>

#include "format.hpp"
#include "lrk_parser/output_helpers.hpp"

namespace lrk_parser {

std::ostream& operator<<(std::ostream& os, const GrammarError& error) {
  switch (error.kind) {
    case GrammarErrorKind::OverlappingAlphabets:
      os << "Terminal and nonterminal overlap: ";
      break;
    case GrammarErrorKind::InvalidStart:
      os << "Start symbol is not a nonterminal: ";
      break;
    case GrammarErrorKind::InvalidLhs:
      os << "Rule has an invalid left-hand side: ";
      break;
    case GrammarErrorKind::UnknownRhsSymbol:
      os << "Rule uses an unknown symbol: ";
      break;
    case GrammarErrorKind::MissingProduction:
      os << "Nonterminal has no production: ";
      break;
    default:
      os << "Unknown grammar error: ";
      break;
  }
  output::PrintSymbol(os, error.symbol);
  if (error.rule_index) {
    os << " (rule #" << *error.rule_index << ')';
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const RuleSyntaxError& error) {
  return os << "Invalid rule syntax (rule #" << error.rule_index << ')';
}

std::ostream& operator<<(std::ostream& os, const TextGrammarError& error) {
  return std::visit(
      [&os](const auto& value) -> std::ostream& { return os << value; }, error);
}

namespace details {
namespace {

void PrintSource(std::ostream& os, const ActionSource& source) {
  os << source.action << ", rule #" << std::to_underlying(source.situation.rule)
     << " (" << source.rule << "), dot " << source.situation.dot
     << ", lookahead ";
  output::PrintQuoted(os, source.situation.lookahead);
}

}  // namespace

std::ostream& operator<<(std::ostream& os, const ActionConflict& error) {
  os << "LR(" << error.k << ") conflict in state "
     << std::to_underlying(error.state) << " on ";
  output::PrintQuoted(os, error.lookahead);
  os << ":\n  existing: ";
  PrintSource(os, error.existing);
  os << "\n  incoming: ";
  PrintSource(os, error.incoming);
  return os;
}

}  // namespace details

std::ostream& operator<<(std::ostream& os, const InvalidLookahead& error) {
  return os << "Invalid lookahead k=" << error.k << " (expected k >= 1)";
}

std::ostream& operator<<(std::ostream& os, const CompileError& error) {
  return std::visit(
      [&os](const auto& value) -> std::ostream& { return os << value; }, error);
}

}  // namespace lrk_parser

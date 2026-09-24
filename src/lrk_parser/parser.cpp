#include "lrk_parser/parser.hpp"

#include <utility>
#include <variant>
#include <vector>

#include "lrk_parser/canonical_collection.hpp"
#include "lrk_parser/first_k.hpp"
#include "lrk_parser/tables_base.hpp"

namespace lrk_parser {

Parser::Parser(std::size_t k, details::PreparedGrammar grammar,
               details::ActionTable actions, details::GotoTable gotos)
    : k_{k},
      grammar_{std::move(grammar)},
      goto_table_{std::move(gotos)},
      action_table_{std::move(actions)} {}

std::expected<Parser, CompileError> Parser::Compile(const Grammar& grammar,
                                                    std::size_t k) {
  if (k == 0) {
    return std::unexpected{CompileError{InvalidLookahead{k}}};
  }

  details::PreparedGrammar prepared{grammar};
  const auto first = details::FirstK::Compute(prepared, k);
  auto collection = details::CanonicalCollection::Build(prepared, first);
  auto actions = details::ActionTable::Build(prepared, first, collection);
  if (!actions) {
    return std::unexpected{CompileError{std::move(actions.error())}};
  }
  auto gotos = details::GotoTable::Build(
      prepared, std::move(collection).TakeTransitions());
  return Parser{k, std::move(prepared), std::move(*actions), std::move(gotos)};
}

bool Parser::Accepts(StringViewT word) const {
  std::vector<details::StateId> stack{details::StateId{0}};
  std::size_t cursor{0};

  while (true) {
    const details::ActionKey key{stack.back(),
                                 StringT{word.substr(cursor, k_)}};
    if (!action_table_.HasParseAction(key)) {
      return false;
    }
    const auto& action = action_table_.GetParseAction(key).value;
    if (const auto* shift = std::get_if<details::Shift>(&action)) {
      if (cursor == word.size()) {
        return false;
      }
      stack.push_back(shift->next_state);
      ++cursor;
      continue;
    }
    if (const auto* reduce = std::get_if<details::Reduce>(&action)) {
      if (!ApplyReduction(stack, reduce->rule)) {
        return false;
      }
      continue;
    }
    return cursor == word.size();
  }
}

bool Parser::ApplyReduction(std::vector<details::StateId>& stack,
                            details::RuleId rule_id) const {
  const auto& rule = grammar_.GetRule(rule_id);
  if (stack.size() <= rule.rhs.size()) {
    return false;
  }
  stack.resize(stack.size() - rule.rhs.size());
  const details::TransitionKey transition{stack.back(), rule.lhs};
  if (!goto_table_.HasGotoState(transition)) {
    return false;
  }
  stack.push_back(goto_table_.GetGotoState(transition));
  return true;
}

}  // namespace lrk_parser

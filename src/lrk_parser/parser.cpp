#include "lrk_parser/parser.hpp"

#include <utility>
#include <variant>
#include <vector>

#include "lrk_parser/details/canonical_collection.hpp"
#include "lrk_parser/details/first_k.hpp"
#include "lrk_parser/details/tables_base.hpp"

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
    const auto* action = action_table_.FindAction(key);
    if (action == nullptr) {
      return false;
    }
    if (const auto* shift = std::get_if<details::Shift>(&action->value)) {
      if (cursor == word.size()) {
        return false;
      }
      stack.push_back(shift->next_state);
      ++cursor;
      continue;
    }
    if (const auto* reduce = std::get_if<details::Reduce>(&action->value)) {
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
  const auto* target = goto_table_.FindState(transition);
  if (target == nullptr) {
    return false;
  }
  stack.push_back(*target);
  return true;
}

}  // namespace lrk_parser

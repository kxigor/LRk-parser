#include "lrk_parser/action_table.hpp"

#include <optional>
#include <span>
#include <unordered_map>
#include <utility>

namespace lrk_parser::details {
namespace {

ActionSource MakeSource(const PreparedGrammar& grammar, Action action,
                        const Situation& situation) {
  return {std::move(action), situation, grammar.GetRule(situation.rule)};
}

}  // namespace

std::expected<ActionTable, ActionConflict> ActionTable::Build(
    const PreparedGrammar& grammar, const FirstK& first,
    const CanonicalCollection& collection) {
  ActionTable result;
  std::unordered_map<ActionKey, Situation, ActionKeyHash> origins;

  auto insert =
      [&](StateId state, StringT lookahead, Action action,
          const Situation& situation) -> std::optional<ActionConflict> {
    ActionKey key{state, std::move(lookahead)};
    const auto [it, inserted] = result.action_table_.try_emplace(key, action);
    if (inserted) {
      origins.emplace(std::move(key), situation);
      return std::nullopt;
    }
    if (it->second == action) {
      return std::nullopt;
    }
    return ActionConflict{first.Lookahead(), state, key.lookahead,
                          MakeSource(grammar, it->second, origins.at(key)),
                          MakeSource(grammar, std::move(action), situation)};
  };

  const auto& states = collection.States();
  const auto& transitions = collection.Transitions();
  for (std::size_t index = 0; index < states.size(); ++index) {
    const StateId state{index};
    for (const auto& situation : states[index]) {
      const auto& rule = grammar.GetRule(situation.rule);
      if (situation.dot < rule.rhs.size()) {
        const auto symbol = rule.rhs[situation.dot];
        if (!grammar.IsTerminal(symbol)) {
          continue;
        }
        const auto target = transitions.at({state, symbol});
        const auto tail = std::span{rule.rhs}.subspan(situation.dot);
        for (const auto& lookahead :
             first.ForSequence(tail, situation.lookahead)) {
          if (auto conflict =
                  insert(state, lookahead, Action{Shift{target}}, situation)) {
            return std::unexpected{std::move(*conflict)};
          }
        }
      } else if (situation.rule == kStartRule) {
        if (situation.lookahead.empty()) {
          if (auto conflict = insert(state, situation.lookahead,
                                     Action{Accept{}}, situation)) {
            return std::unexpected{std::move(*conflict)};
          }
        }
      } else {
        if (auto conflict = insert(state, situation.lookahead,
                                   Action{Reduce{situation.rule}}, situation)) {
          return std::unexpected{std::move(*conflict)};
        }
      }
    }
  }
  return result;
}

bool ActionTable::HasParseAction(const ActionKey& key) const {
  return action_table_.contains(key);
}

const Action& ActionTable::GetParseAction(const ActionKey& key) const {
  return action_table_.at(key);
}

}  // namespace lrk_parser::details

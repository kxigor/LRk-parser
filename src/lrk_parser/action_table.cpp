#include "lrk_parser/details/action_table.hpp"

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

class ActionTable::Builder {
 public:
  Builder(const PreparedGrammar& grammar, const FirstK& first,
          const CanonicalCollection& collection)
      : grammar_{grammar}, first_{first}, collection_{collection} {}

  std::expected<ActionTable, ActionConflict> Run() {
    const auto& states = collection_.States();
    for (std::size_t index = 0; index < states.size(); ++index) {
      for (const auto& situation : states[index]) {
        if (auto conflict = ProcessSituation(StateId{index}, situation)) {
          return std::unexpected{std::move(*conflict)};
        }
      }
    }
    return std::move(table_);
  }

 private:
  std::optional<ActionConflict> Insert(StateId state, StringT lookahead,
                                       Action action,
                                       const Situation& situation) {
    ActionKey key{state, std::move(lookahead)};
    const auto [it, inserted] = table_.action_table_.try_emplace(key, action);
    if (inserted) {
      origins_.emplace(std::move(key), situation);
      return std::nullopt;
    }
    if (it->second == action) {
      return std::nullopt;
    }
    return ActionConflict{first_.Lookahead(), state, key.lookahead,
                          MakeSource(grammar_, it->second, origins_.at(key)),
                          MakeSource(grammar_, std::move(action), situation)};
  }

  std::optional<ActionConflict> ProcessSituation(StateId state,
                                                 const Situation& situation) {
    const auto& rule = grammar_.GetRule(situation.rule);
    if (situation.dot >= rule.rhs.size()) {
      if (situation.rule == kStartRule) {
        if (situation.lookahead.empty()) {
          return Insert(state, situation.lookahead, Action{Accept{}},
                        situation);
        }
        return std::nullopt;
      }
      return Insert(state, situation.lookahead, Action{Reduce{situation.rule}},
                    situation);
    }

    const auto symbol = rule.rhs[situation.dot];
    if (!grammar_.IsTerminal(symbol)) {
      return std::nullopt;
    }
    const auto target = collection_.Transitions().at({state, symbol});
    const auto tail = std::span{rule.rhs}.subspan(situation.dot);
    for (const auto& lookahead :
         first_.ForSequence(tail, situation.lookahead)) {
      if (auto conflict =
              Insert(state, lookahead, Action{Shift{target}}, situation)) {
        return conflict;
      }
    }
    return std::nullopt;
  }

  const PreparedGrammar& grammar_;
  const FirstK& first_;
  const CanonicalCollection& collection_;
  ActionTable table_;
  std::unordered_map<ActionKey, Situation, ActionKeyHash> origins_;
};

std::expected<ActionTable, ActionConflict> ActionTable::Build(
    const PreparedGrammar& grammar, const FirstK& first,
    const CanonicalCollection& collection) {
  return Builder{grammar, first, collection}.Run();
}

const Action* ActionTable::FindAction(const ActionKey& key) const {
  const auto it = action_table_.find(key);
  return it == action_table_.end() ? nullptr : &it->second;
}

}  // namespace lrk_parser::details

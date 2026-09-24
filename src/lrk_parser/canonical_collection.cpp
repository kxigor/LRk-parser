#include "lrk_parser/canonical_collection.hpp"

#include <algorithm>
#include <deque>
#include <span>
#include <tuple>
#include <unordered_set>

namespace lrk_parser::details {
namespace {

bool SituationLess(const Situation& lhs, const Situation& rhs) {
  return std::tie(lhs.rule, lhs.dot, lhs.lookahead) <
         std::tie(rhs.rule, rhs.dot, rhs.lookahead);
}

struct SituationHash {
  std::size_t operator()(const Situation& situation) const noexcept {
    std::size_t seed = 0;
    seed ^= std::to_underlying(situation.rule) + 0x9e3779b9 + (seed << 6) +
            (seed >> 2);
    seed ^= situation.dot + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<StringT>{}(situation.lookahead) + 0x9e3779b9 +
            (seed << 6) + (seed >> 2);
    return seed;
  }
};

struct SituationsHash {
  std::size_t operator()(const Situations& state) const noexcept {
    std::size_t seed = state.size();
    for (const auto& situation : state) {
      seed += SituationHash{}(situation);
    }
    return seed;
  }
};

Situations Closure(const PreparedGrammar& grammar, const FirstK& first,
                   Situations kernel) {
  std::unordered_set<Situation, SituationHash> visited{kernel.begin(),
                                                       kernel.end()};
  std::deque<Situation> pending{visited.begin(), visited.end()};
  while (!pending.empty()) {
    const auto situation = std::move(pending.front());
    pending.pop_front();
    const auto& rhs = grammar.GetRule(situation.rule).rhs;
    if (situation.dot >= rhs.size()) {
      continue;
    }
    const auto symbol = rhs[situation.dot];
    if (!grammar.IsNonterminal(symbol)) {
      continue;
    }

    const auto suffix = std::span{rhs}.subspan(situation.dot + 1);
    const auto lookaheads = first.ForSequence(suffix, situation.lookahead);
    for (auto rule : grammar.RulesFor(symbol)) {
      for (const auto& lookahead : lookaheads) {
        Situation next{rule, 0, lookahead};
        if (visited.insert(next).second) {
          pending.push_back(std::move(next));
        }
      }
    }
  }
  Situations result{visited.begin(), visited.end()};
  std::ranges::sort(result, SituationLess);
  return result;
}

Situations GoTo(const PreparedGrammar& grammar, const FirstK& first,
                std::span<const Situation> state, SymbolId symbol) {
  Situations kernel;
  for (const auto& situation : state) {
    const auto& rhs = grammar.GetRule(situation.rule).rhs;
    if (situation.dot < rhs.size() && rhs[situation.dot] == symbol) {
      kernel.push_back(
          {situation.rule, situation.dot + 1, situation.lookahead});
    }
  }
  return Closure(grammar, first, std::move(kernel));
}

}  // namespace

CanonicalCollection CanonicalCollection::Build(const PreparedGrammar& grammar,
                                               const FirstK& first) {
  CanonicalCollection result;
  std::unordered_map<Situations, StateId, SituationsHash> state_ids;
  auto intern = [&](Situations state) {
    const auto [it, inserted] =
        state_ids.try_emplace(state, StateId{result.states_.size()});
    if (inserted) {
      result.states_.push_back(std::move(state));
    }
    return it->second;
  };
  intern(Closure(grammar, first, {{kStartRule, 0, {}}}));

  for (std::size_t index = 0; index < result.states_.size(); ++index) {
    auto add_transition = [&](SymbolId symbol) {
      auto next = GoTo(grammar, first, result.states_[index], symbol);
      if (next.empty()) {
        return;
      }
      const auto target = intern(std::move(next));
      result.transitions_.emplace(TransitionKey{StateId{index}, symbol},
                                  target);
    };
    for (auto symbol : grammar.Terminals()) {
      add_transition(symbol);
    }
    for (auto symbol : grammar.Nonterminals()) {
      add_transition(symbol);
    }
  }
  return result;
}

}  // namespace lrk_parser::details

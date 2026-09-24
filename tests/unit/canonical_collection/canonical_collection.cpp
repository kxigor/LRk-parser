#include "lrk_parser/canonical_collection.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <span>
#include <sstream>
#include <tuple>
#include <utility>
#include <vector>

#include "lrk_parser/output_helpers.hpp"

namespace lrk_parser::details {
namespace {

bool SituationLess(const Situation& lhs, const Situation& rhs) {
  return std::tie(lhs.rule, lhs.dot, lhs.lookahead) <
         std::tie(rhs.rule, rhs.dot, rhs.lookahead);
}

CanonicalCollection Build(GrammarSpec spec, std::size_t k = 1) {
  const PreparedGrammar grammar{MakeGrammar(std::move(spec)).value()};
  const auto first = FirstK::Compute(grammar, k);
  return CanonicalCollection::Build(grammar, first);
}

StateId Next(const CanonicalCollection& collection, StateId state,
             CharT symbol) {
  return collection.Transitions().at({state, EncodeSymbol(symbol)});
}

const Situations& After(const CanonicalCollection& collection, StateId state,
                        CharT symbol) {
  return collection
      .States()[std::to_underlying(Next(collection, state, symbol))];
}

struct Graph {
  bool operator==(const Graph& other) const {
    return std::ranges::is_permutation(states, other.states) &&
           std::ranges::is_permutation(edges, other.edges);
  }

  std::vector<Situations> states;
  std::vector<std::tuple<Situations, SymbolId, Situations>> edges;
};

Graph Capture(const CanonicalCollection& collection,
              std::span<const RuleId> original_rules = {}) {
  Graph graph;
  for (auto state : collection.States()) {
    if (!original_rules.empty()) {
      for (auto& situation : state) {
        situation.rule = original_rules[std::to_underlying(situation.rule)];
      }
      std::ranges::sort(state, SituationLess);
    }
    graph.states.push_back(std::move(state));
  }
  for (const auto& [key, target] : collection.Transitions()) {
    graph.edges.emplace_back(
        graph.states[std::to_underlying(key.current_state_id)], key.symbol,
        graph.states[std::to_underlying(target)]);
  }
  return graph;
}

TEST(CanonicalCollection, BuildsSimpleGrammarByStateContents) {
  const auto collection = Build({"a", "SA", {{'S', "A"}, {'A', "a"}}, 'S'});
  const Situations initial{
      {kStartRule, 0, ""}, {RuleId{1}, 0, ""}, {RuleId{2}, 0, ""}};

  ASSERT_EQ(collection.States().size(), 4);
  EXPECT_EQ(collection.States().front(), initial);
  EXPECT_EQ(After(collection, StateId{0}, 'a'),
            (Situations{{RuleId{2}, 1, ""}}));
  EXPECT_EQ(After(collection, StateId{0}, 'A'),
            (Situations{{RuleId{1}, 1, ""}}));
  EXPECT_EQ(After(collection, StateId{0}, 'S'),
            (Situations{{kStartRule, 1, ""}}));
  EXPECT_EQ(collection.Transitions().size(), 3);
  EXPECT_FALSE(
      collection.Transitions().contains({StateId{0}, EncodeSymbol('b')}));
}

TEST(CanonicalCollection, PropagatesShortLookaheadThroughNullableChain) {
  for (std::size_t k : {1U, 2U, 3U}) {
    const auto collection = Build(
        {"abc", "SAB", {{'S', "Abc"}, {'A', "B"}, {'B', "a"}, {'B', ""}}, 'S'},
        k);
    const auto lookahead = StringT{"bc"}.substr(0, k);
    const Situations initial{{kStartRule, 0, ""},
                             {RuleId{1}, 0, ""},
                             {RuleId{2}, 0, lookahead},
                             {RuleId{3}, 0, lookahead},
                             {RuleId{4}, 0, lookahead}};

    EXPECT_EQ(collection.States().front(), initial);
    EXPECT_EQ(After(collection, StateId{0}, 'a'),
              (Situations{{RuleId{3}, 1, lookahead}}));
    EXPECT_EQ(After(collection, StateId{0}, 'B'),
              (Situations{{RuleId{2}, 1, lookahead}}));
  }
}

TEST(CanonicalCollection, ClosesUnitCycleWithoutDuplicateSituations) {
  const auto collection =
      Build({"a", "SA", {{'S', "A"}, {'A', "A"}, {'A', "a"}}, 'S'});
  const Situations initial{{kStartRule, 0, ""},
                           {RuleId{1}, 0, ""},
                           {RuleId{2}, 0, ""},
                           {RuleId{3}, 0, ""}};

  EXPECT_EQ(collection.States().front(), initial);
  EXPECT_EQ(After(collection, StateId{0}, 'A'),
            (Situations{{RuleId{1}, 1, ""}, {RuleId{2}, 1, ""}}));
}

TEST(CanonicalCollection, KeepsDifferentLookaheadsForTheSameRuleAndDot) {
  const auto collection =
      Build({"abc", "SA", {{'S', "Aa"}, {'S', "Ab"}, {'A', "c"}}, 'S'});
  const Situations initial{{kStartRule, 0, ""},
                           {RuleId{1}, 0, ""},
                           {RuleId{2}, 0, ""},
                           {RuleId{3}, 0, "a"},
                           {RuleId{3}, 0, "b"}};

  EXPECT_EQ(collection.States().front(), initial);
  EXPECT_EQ(After(collection, StateId{0}, 'c'),
            (Situations{{RuleId{3}, 1, "a"}, {RuleId{3}, 1, "b"}}));
}

TEST(CanonicalCollection, KeepsCanonicalStatesWithTheSameCoreSeparate) {
  const auto collection = Build({"abcde",
                                 "SAB",
                                 {{'S', "aAd"},
                                  {'S', "bAe"},
                                  {'S', "aBe"},
                                  {'S', "bBd"},
                                  {'A', "c"},
                                  {'B', "c"}},
                                 'S'});
  const auto after_a = Next(collection, StateId{0}, 'a');
  const auto after_b = Next(collection, StateId{0}, 'b');

  EXPECT_NE(Next(collection, after_a, 'c'), Next(collection, after_b, 'c'));
  EXPECT_EQ(After(collection, after_a, 'c'),
            (Situations{{RuleId{5}, 1, "d"}, {RuleId{6}, 1, "e"}}));
  EXPECT_EQ(After(collection, after_b, 'c'),
            (Situations{{RuleId{5}, 1, "e"}, {RuleId{6}, 1, "d"}}));
}

TEST(CanonicalCollection, KeepsDuplicateProductionsAsDifferentRules) {
  const auto collection = Build({"a", "S", {{'S', "a"}, {'S', "a"}}, 'S'});

  EXPECT_EQ(After(collection, StateId{0}, 'a'),
            (Situations{{RuleId{1}, 1, ""}, {RuleId{2}, 1, ""}}));
}

TEST(CanonicalCollection,
     StoresSortedUniqueStatesAndReusesRecursiveTransitions) {
  const auto collection =
      Build({"cd", "SC", {{'S', "CC"}, {'C', "cC"}, {'C', "d"}}, 'S'});
  const auto after_c = Next(collection, StateId{0}, 'c');

  EXPECT_EQ(Next(collection, after_c, 'c'), after_c);
  const auto& states = collection.States();
  for (const auto& state : states) {
    EXPECT_FALSE(state.empty());
    EXPECT_TRUE(std::ranges::is_sorted(state, SituationLess));
    EXPECT_EQ(std::ranges::adjacent_find(state), state.end());
  }
  for (const auto& state : states) {
    EXPECT_EQ(std::ranges::count(states, state), 1);
  }
  for (const auto& [key, target] : collection.Transitions()) {
    EXPECT_LT(std::to_underlying(key.current_state_id), states.size());
    EXPECT_LT(std::to_underlying(target), states.size());
  }
}

TEST(CanonicalCollection, RuleAndAlphabetOrderDoNotChangeGraph) {
  GrammarSpec spec{"cd", "SC", {{'S', "CC"}, {'C', "cC"}, {'C', "d"}}, 'S'};
  const auto rules = spec.rules;
  for (std::size_t k : {1U, 2U, 3U}) {
    std::array<std::size_t, 3> order{0, 1, 2};
    const auto reference = Capture(Build(spec, k));
    std::ranges::reverse(spec.terminals);
    std::ranges::reverse(spec.nonterminals);
    do {
      std::vector<RuleId> original_rules{kStartRule};
      for (std::size_t index = 0; index < order.size(); ++index) {
        spec.rules[index] = rules[order[index]];
        original_rules.push_back(RuleId{order[index] + 1});
      }
      const auto collection = Build(spec, k);
      EXPECT_EQ(Capture(collection, original_rules), reference);
    } while (std::next_permutation(order.begin(), order.end()));
    spec.rules = rules;
  }
}

TEST(CanonicalCollection,
     OwnsGraphAfterInputsAreDestroyedAndCanTransferTransitions) {
  const auto collection = Build({"a", "S", {{'S', "aS"}, {'S', ""}}, 'S'}, 2);
  const auto expected = Capture(collection);
  const auto other = Build({"b", "S", {{'S', "b"}}, 'S'});
  EXPECT_NE(Capture(other), expected);
  auto copy = collection;
  auto moved = std::move(copy);

  EXPECT_EQ(Capture(moved), expected);
  const auto transitions = std::move(moved).TakeTransitions();
  EXPECT_EQ(transitions, collection.Transitions());
  EXPECT_EQ(Capture(collection), expected);

  std::ostringstream output;
  EXPECT_NO_THROW(output << collection);
  EXPECT_NE(output.str().find("Lookahead"), std::string::npos);
}

}  // namespace
}  // namespace lrk_parser::details

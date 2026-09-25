#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "lrk_parser/details/action_table.hpp"
#include "lrk_parser/output.hpp"
#include "lrk_parser/text_grammar.hpp"

namespace lrk_parser::details {
namespace {

TEST(ActionTable, BuildsDistinctShiftReduceAndAcceptActions) {
  const PreparedGrammar grammar{
      ParseGrammar("a", "SA", {"S->A", "A->a"}, 'S').value()};
  const auto first = FirstK::Compute(grammar, 1);
  const auto collection = CanonicalCollection::Build(grammar, first);
  const auto result = ActionTable::Build(grammar, first, collection);
  ASSERT_TRUE(result.has_value());

  const StateId initial{0};
  const auto after_a =
      collection.Transitions().at({initial, EncodeSymbol('a')});
  const auto after_s =
      collection.Transitions().at({initial, EncodeSymbol('S')});

  const auto* shift = result->FindAction({initial, "a"});
  ASSERT_NE(shift, nullptr);
  ASSERT_TRUE(std::holds_alternative<Shift>(shift->value));
  EXPECT_EQ(std::get<Shift>(shift->value).next_state, after_a);

  const auto* reduce = result->FindAction({after_a, ""});
  ASSERT_NE(reduce, nullptr);
  ASSERT_TRUE(std::holds_alternative<Reduce>(reduce->value));
  EXPECT_EQ(std::get<Reduce>(reduce->value).rule, RuleId{2});

  const auto* accept = result->FindAction({after_s, ""});
  ASSERT_NE(accept, nullptr);
  EXPECT_TRUE(std::holds_alternative<Accept>(accept->value));
  EXPECT_EQ(result->FindAction({initial, ""}), nullptr);

  std::ostringstream output;
  output << *shift << ' ' << *reduce << ' ' << *accept;
  EXPECT_EQ(output.str(),
            "S" + std::to_string(std::to_underlying(after_a)) + " R2 ACC");
}

TEST(ActionTable, ReportsOwningReduceReduceConflict) {
  const auto result = [] {
    const PreparedGrammar grammar{
        ParseGrammar("a", "S", {"S->a", "S->a"}, 'S').value()};
    const auto first = FirstK::Compute(grammar, 2);
    const auto collection = CanonicalCollection::Build(grammar, first);
    const auto after_a =
        collection.Transitions().at({StateId{0}, EncodeSymbol('a')});
    auto built = ActionTable::Build(grammar, first, collection);
    EXPECT_FALSE(built.has_value());
    return std::pair{std::move(built.error()), after_a};
  }();

  const auto& [conflict, after_a] = result;
  EXPECT_EQ(conflict.k, 2);
  EXPECT_EQ(conflict.state, after_a);
  EXPECT_EQ(conflict.lookahead, "");
  EXPECT_EQ(conflict.existing.situation.lookahead, "");
  EXPECT_EQ(conflict.incoming.situation.lookahead, "");
  EXPECT_EQ(conflict.existing.situation.dot, 1);
  EXPECT_EQ(conflict.incoming.situation.dot, 1);
  EXPECT_EQ(conflict.existing.rule.lhs, EncodeSymbol('S'));
  EXPECT_EQ(conflict.incoming.rule.lhs, EncodeSymbol('S'));
  EXPECT_EQ(conflict.existing.rule.rhs,
            (std::vector<SymbolId>{EncodeSymbol('a')}));
  EXPECT_EQ(conflict.incoming.rule.rhs, conflict.existing.rule.rhs);

  const auto& first_reduce = std::get<Reduce>(conflict.existing.action.value);
  const auto& second_reduce = std::get<Reduce>(conflict.incoming.action.value);
  EXPECT_EQ(first_reduce.rule, conflict.existing.situation.rule);
  EXPECT_EQ(second_reduce.rule, conflict.incoming.situation.rule);
  EXPECT_NE(first_reduce.rule, second_reduce.rule);
}

TEST(ActionTable, ReportsShiftReduceConflictWithRuleSources) {
  const PreparedGrammar grammar{
      ParseGrammar("a", "S", {"S->SS", "S->a"}, 'S').value()};
  const auto first = FirstK::Compute(grammar, 1);
  const auto collection = CanonicalCollection::Build(grammar, first);
  const auto result = ActionTable::Build(grammar, first, collection);
  ASSERT_FALSE(result.has_value());

  const auto& conflict = result.error();
  EXPECT_EQ(conflict.k, 1);
  EXPECT_EQ(conflict.lookahead, "a");
  EXPECT_NE(conflict.existing.action, conflict.incoming.action);
  EXPECT_TRUE(std::holds_alternative<Shift>(conflict.existing.action.value) ||
              std::holds_alternative<Shift>(conflict.incoming.action.value));
  EXPECT_TRUE(std::holds_alternative<Reduce>(conflict.existing.action.value) ||
              std::holds_alternative<Reduce>(conflict.incoming.action.value));
  EXPECT_EQ(conflict.existing.rule.lhs,
            grammar.GetRule(conflict.existing.situation.rule).lhs);
  EXPECT_EQ(conflict.existing.rule.rhs,
            grammar.GetRule(conflict.existing.situation.rule).rhs);
  EXPECT_EQ(conflict.incoming.rule.lhs,
            grammar.GetRule(conflict.incoming.situation.rule).lhs);
  EXPECT_EQ(conflict.incoming.rule.rhs,
            grammar.GetRule(conflict.incoming.situation.rule).rhs);
}

TEST(ActionTable, ReportsAcceptReduceConflict) {
  const PreparedGrammar grammar{ParseGrammar("a", "S", {"S->S"}, 'S').value()};
  const auto first = FirstK::Compute(grammar, 1);
  const auto collection = CanonicalCollection::Build(grammar, first);
  const auto result = ActionTable::Build(grammar, first, collection);
  ASSERT_FALSE(result.has_value());

  const auto& conflict = result.error();
  const auto after_s =
      collection.Transitions().at({StateId{0}, EncodeSymbol('S')});
  EXPECT_EQ(conflict.state, after_s);
  EXPECT_EQ(conflict.lookahead, "");
  EXPECT_TRUE(std::holds_alternative<Accept>(conflict.existing.action.value) ||
              std::holds_alternative<Accept>(conflict.incoming.action.value));
  EXPECT_TRUE(std::holds_alternative<Reduce>(conflict.existing.action.value) ||
              std::holds_alternative<Reduce>(conflict.incoming.action.value));
}

TEST(ActionTable, CanCopyAndMoveBuiltTable) {
  const PreparedGrammar grammar{
      ParseGrammar("a", "SA", {"S->A", "A->a"}, 'S').value()};
  const auto first = FirstK::Compute(grammar, 1);
  const auto collection = CanonicalCollection::Build(grammar, first);
  auto built = ActionTable::Build(grammar, first, collection);
  ASSERT_TRUE(built.has_value());

  const auto copy = *built;
  const auto moved = std::move(*built);
  const ActionKey key{StateId{0}, "a"};
  ASSERT_NE(copy.FindAction(key), nullptr);
  ASSERT_NE(moved.FindAction(key), nullptr);
  EXPECT_EQ(*copy.FindAction(key), *moved.FindAction(key));
}

}  // namespace
}  // namespace lrk_parser::details

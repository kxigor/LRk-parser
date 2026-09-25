#include <gtest/gtest.h>

#include <memory>
#include <utility>
#include <vector>

#include "lrk_parser/config.hpp"
#include "lrk_parser/details/canonical_collection.hpp"
#include "lrk_parser/details/first_k.hpp"
#include "lrk_parser/details/goto_table.hpp"
#include "lrk_parser/grammar.hpp"

using namespace lrk_parser;
using namespace lrk_parser::details;

class CanonicalCollectionBaseTest : public ::testing::Test {
 protected:
  void SetUpSimpleGrammar() {
    StringT terminals = "a";
    StringT nonterminals = "SA";
    CharT start = 'S';
    std::vector<StringT> rules{"S->A", "A->a"};
    grammar_ = std::make_unique<PreparedGrammar>(
        Grammar::FromTextRules(terminals, nonterminals, rules, start).value());
    first_k_ = std::make_unique<FirstK>(FirstK::Compute(*grammar_, 1));
  }

  std::unique_ptr<PreparedGrammar> grammar_;
  std::unique_ptr<FirstK> first_k_;
};

class GotoTableTest : public CanonicalCollectionBaseTest {
 protected:
  TransitionKey CreateTransitionKey(StateId current_state, CharT symbol) const {
    return TransitionKey{.current_state_id = current_state,
                         .symbol = EncodeSymbol(symbol)};
  }
};

const StateId source_id{0};
const CharT symbol_s = 'S';

TEST_F(GotoTableTest, DefaultConstructor) {
  GotoTable gt;
  TransitionKey key = CreateTransitionKey(StateId{0}, 'S');
  EXPECT_EQ(gt.FindState(key), nullptr);
}

TEST_F(GotoTableTest, BuildCopiesTransitions) {
  SetUpSimpleGrammar();
  auto cc_source = CanonicalCollection::Build(*grammar_, *first_k_);

  const auto expected =
      cc_source.Transitions().at(CreateTransitionKey(source_id, symbol_s));
  GotoTable gt = GotoTable::Build(*grammar_, cc_source.Transitions());

  TransitionKey key = CreateTransitionKey(source_id, symbol_s);

  ASSERT_NE(gt.FindState(key), nullptr);
  EXPECT_EQ(*gt.FindState(key), expected);
}

TEST_F(GotoTableTest, BuildMovesTransitions) {
  SetUpSimpleGrammar();

  auto cc_temp = CanonicalCollection::Build(*grammar_, *first_k_);
  const auto expected =
      cc_temp.Transitions().at(CreateTransitionKey(source_id, symbol_s));
  GotoTable gt =
      GotoTable::Build(*grammar_, std::move(cc_temp).TakeTransitions());

  TransitionKey key = CreateTransitionKey(source_id, symbol_s);

  ASSERT_NE(gt.FindState(key), nullptr);
  EXPECT_EQ(*gt.FindState(key), expected);
}

TEST_F(GotoTableTest, FindsOnlyNonterminalTransitions) {
  SetUpSimpleGrammar();
  auto cc = CanonicalCollection::Build(*grammar_, *first_k_);
  GotoTable gt = GotoTable::Build(*grammar_, cc.Transitions());

  TransitionKey existing_key = CreateTransitionKey(source_id, symbol_s);
  ASSERT_NE(gt.FindState(existing_key), nullptr);

  TransitionKey non_existing_key =
      CreateTransitionKey(*gt.FindState(existing_key), symbol_s);
  EXPECT_EQ(gt.FindState(non_existing_key), nullptr);

  TransitionKey unused_symbol_key = CreateTransitionKey(source_id, 'b');
  EXPECT_EQ(gt.FindState(unused_symbol_key), nullptr);

  TransitionKey terminal_key = CreateTransitionKey(source_id, 'a');
  ASSERT_TRUE(cc.Transitions().contains(terminal_key));
  EXPECT_EQ(gt.FindState(terminal_key), nullptr);
}

TEST_F(GotoTableTest, ReturnsTargetStateForExistingTransition) {
  SetUpSimpleGrammar();
  auto cc = CanonicalCollection::Build(*grammar_, *first_k_);
  GotoTable gt = GotoTable::Build(*grammar_, cc.Transitions());

  TransitionKey existing_key = CreateTransitionKey(source_id, symbol_s);
  const StateId* target_state = gt.FindState(existing_key);

  ASSERT_NE(target_state, nullptr);
  EXPECT_EQ(*target_state, cc.Transitions().at(existing_key));

  TransitionKey non_existing_key = CreateTransitionKey(*target_state, symbol_s);

  EXPECT_EQ(gt.FindState(non_existing_key), nullptr);
}

TEST_F(GotoTableTest, CopyAssignment) {
  SetUpSimpleGrammar();
  auto cc = CanonicalCollection::Build(*grammar_, *first_k_);
  GotoTable source_gt = GotoTable::Build(*grammar_, cc.Transitions());
  GotoTable dest_gt;

  dest_gt = source_gt;

  TransitionKey key = CreateTransitionKey(source_id, symbol_s);
  ASSERT_NE(dest_gt.FindState(key), nullptr);
  EXPECT_EQ(*dest_gt.FindState(key), cc.Transitions().at(key));
}

TEST_F(GotoTableTest, MoveAssignment) {
  SetUpSimpleGrammar();
  auto cc = CanonicalCollection::Build(*grammar_, *first_k_);
  GotoTable source_gt = GotoTable::Build(*grammar_, cc.Transitions());
  GotoTable dest_gt;

  dest_gt = std::move(source_gt);

  TransitionKey key = CreateTransitionKey(source_id, symbol_s);
  ASSERT_NE(dest_gt.FindState(key), nullptr);
  EXPECT_EQ(*dest_gt.FindState(key), cc.Transitions().at(key));
}

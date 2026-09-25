#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "lrk_parser/config.hpp"
#include "lrk_parser/details/canonical_collection.hpp"
#include "lrk_parser/details/first_k.hpp"
#include "lrk_parser/details/goto_table.hpp"
#include "lrk_parser/text_grammar.hpp"

using namespace lrk_parser;
using namespace lrk_parser::details;

class CanonicalCollectionBaseTest : public ::testing::Test {
 protected:
  void SetUpSimpleGrammar() {
    StringT T = "a";
    StringT N = "SA";
    CharT Start = 'S';
    std::vector<StringT> rules{"S->A", "A->a"};
    grammar_ = PreparedGrammar{ParseGrammar(T, N, rules, Start).value()};
    first_k_ = std::make_unique<FirstK>(FirstK::Compute(*grammar_, 1));
  }

  std::optional<PreparedGrammar> grammar_;
  std::unique_ptr<FirstK> first_k_;
};

class GotoTableTest : public CanonicalCollectionBaseTest {
 protected:
  TransitionKey create_t_key(StateId current_state, CharT symbol) const {
    return TransitionKey{.current_state_id = current_state,
                         .symbol = EncodeSymbol(symbol)};
  }
};

const StateId TEST_SOURCE_ID{0};
const CharT TEST_SYMBOL_S = 'S';

TEST_F(GotoTableTest, DefaultConstructor) {
  GotoTable gt;
  TransitionKey key = create_t_key(StateId{0}, 'S');
  EXPECT_EQ(gt.FindState(key), nullptr);
}

TEST_F(GotoTableTest, BuildCopiesTransitions) {
  SetUpSimpleGrammar();
  auto cc_source = CanonicalCollection::Build(*grammar_, *first_k_);

  const auto expected =
      cc_source.Transitions().at(create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S));
  GotoTable gt = GotoTable::Build(*grammar_, cc_source.Transitions());

  TransitionKey key = create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S);

  ASSERT_NE(gt.FindState(key), nullptr);
  EXPECT_EQ(*gt.FindState(key), expected);
}

TEST_F(GotoTableTest, BuildMovesTransitions) {
  SetUpSimpleGrammar();

  auto cc_temp = CanonicalCollection::Build(*grammar_, *first_k_);
  const auto expected =
      cc_temp.Transitions().at(create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S));
  GotoTable gt =
      GotoTable::Build(*grammar_, std::move(cc_temp).TakeTransitions());

  TransitionKey key = create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S);

  ASSERT_NE(gt.FindState(key), nullptr);
  EXPECT_EQ(*gt.FindState(key), expected);
}

TEST_F(GotoTableTest, FindsOnlyNonterminalTransitions) {
  SetUpSimpleGrammar();
  auto cc = CanonicalCollection::Build(*grammar_, *first_k_);
  GotoTable gt = GotoTable::Build(*grammar_, cc.Transitions());

  TransitionKey existing_key = create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S);
  ASSERT_NE(gt.FindState(existing_key), nullptr);

  TransitionKey non_existing_key =
      create_t_key(*gt.FindState(existing_key), TEST_SYMBOL_S);
  EXPECT_EQ(gt.FindState(non_existing_key), nullptr);

  TransitionKey unused_symbol_key = create_t_key(TEST_SOURCE_ID, 'b');
  EXPECT_EQ(gt.FindState(unused_symbol_key), nullptr);

  TransitionKey terminal_key = create_t_key(TEST_SOURCE_ID, 'a');
  ASSERT_TRUE(cc.Transitions().contains(terminal_key));
  EXPECT_EQ(gt.FindState(terminal_key), nullptr);
}

TEST_F(GotoTableTest, ReturnsTargetStateForExistingTransition) {
  SetUpSimpleGrammar();
  auto cc = CanonicalCollection::Build(*grammar_, *first_k_);
  GotoTable gt = GotoTable::Build(*grammar_, cc.Transitions());

  TransitionKey existing_key = create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S);
  const StateId* target_state = gt.FindState(existing_key);

  ASSERT_NE(target_state, nullptr);
  EXPECT_EQ(*target_state, cc.Transitions().at(existing_key));

  TransitionKey non_existing_key = create_t_key(*target_state, TEST_SYMBOL_S);

  EXPECT_EQ(gt.FindState(non_existing_key), nullptr);
}

TEST_F(GotoTableTest, CopyAssignment) {
  SetUpSimpleGrammar();
  auto cc = CanonicalCollection::Build(*grammar_, *first_k_);
  GotoTable source_gt = GotoTable::Build(*grammar_, cc.Transitions());
  GotoTable dest_gt;

  dest_gt = source_gt;

  TransitionKey key = create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S);
  ASSERT_NE(dest_gt.FindState(key), nullptr);
  EXPECT_EQ(*dest_gt.FindState(key), cc.Transitions().at(key));
}

TEST_F(GotoTableTest, MoveAssignment) {
  SetUpSimpleGrammar();
  auto cc = CanonicalCollection::Build(*grammar_, *first_k_);
  GotoTable source_gt = GotoTable::Build(*grammar_, cc.Transitions());
  GotoTable dest_gt;

  dest_gt = std::move(source_gt);

  TransitionKey key = create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S);
  ASSERT_NE(dest_gt.FindState(key), nullptr);
  EXPECT_EQ(*dest_gt.FindState(key), cc.Transitions().at(key));
}

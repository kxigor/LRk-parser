#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

#include "lrk_parser/canonical_collection.hpp"
#include "lrk_parser/config.hpp"
#include "lrk_parser/first_k.hpp"
#include "lrk_parser/goto_table.hpp"
#include "lrk_parser/text_grammar.hpp"

using namespace lrk_parser;
using namespace lrk_parser::details;

class CanonicalCollectionBaseTest : public ::testing::Test {
 protected:
  void SetUpSimpleGrammar() {
    StringT T = "a";
    StringT N = "SA";
    CharT Start = 'S';
    VectorT<StringT> rules = {"S->A", "A->a"};
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
  EXPECT_FALSE(gt.HasGotoState(key));
}

TEST_F(GotoTableTest, BuildCopiesTransitions) {
  SetUpSimpleGrammar();
  auto cc_source = CanonicalCollection::Build(*grammar_, *first_k_);

  const auto expected =
      cc_source.Transitions().at(create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S));
  GotoTable gt = GotoTable::Build(*grammar_, cc_source.Transitions());

  TransitionKey key = create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S);

  ASSERT_TRUE(gt.HasGotoState(key));
  EXPECT_EQ(gt.GetGotoState(key), expected);
}

TEST_F(GotoTableTest, BuildMovesTransitions) {
  SetUpSimpleGrammar();

  auto cc_temp = CanonicalCollection::Build(*grammar_, *first_k_);
  const auto expected =
      cc_temp.Transitions().at(create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S));
  GotoTable gt =
      GotoTable::Build(*grammar_, std::move(cc_temp).TakeTransitions());

  TransitionKey key = create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S);

  ASSERT_TRUE(gt.HasGotoState(key));
  EXPECT_EQ(gt.GetGotoState(key), expected);
}

TEST_F(GotoTableTest, HasGotoStateLogic) {
  SetUpSimpleGrammar();
  auto cc = CanonicalCollection::Build(*grammar_, *first_k_);
  GotoTable gt = GotoTable::Build(*grammar_, cc.Transitions());

  TransitionKey existing_key = create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S);
  EXPECT_TRUE(gt.HasGotoState(existing_key));

  TransitionKey non_existing_key =
      create_t_key(gt.GetGotoState(existing_key), TEST_SYMBOL_S);
  EXPECT_FALSE(gt.HasGotoState(non_existing_key));

  TransitionKey unused_symbol_key = create_t_key(TEST_SOURCE_ID, 'b');
  EXPECT_FALSE(gt.HasGotoState(unused_symbol_key));

  TransitionKey terminal_key = create_t_key(TEST_SOURCE_ID, 'a');
  ASSERT_TRUE(cc.Transitions().contains(terminal_key));
  EXPECT_FALSE(gt.HasGotoState(terminal_key));
}

TEST_F(GotoTableTest, GetGotoStateLogic) {
  SetUpSimpleGrammar();
  auto cc = CanonicalCollection::Build(*grammar_, *first_k_);
  GotoTable gt = GotoTable::Build(*grammar_, cc.Transitions());

  TransitionKey existing_key = create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S);
  const StateId& target_state = gt.GetGotoState(existing_key);

  EXPECT_EQ(target_state, cc.Transitions().at(existing_key));

  TransitionKey non_existing_key =
      create_t_key(gt.GetGotoState(existing_key), TEST_SYMBOL_S);

  EXPECT_THROW((void)gt.GetGotoState(non_existing_key), std::out_of_range);
}

TEST_F(GotoTableTest, CopyAssignment) {
  SetUpSimpleGrammar();
  auto cc = CanonicalCollection::Build(*grammar_, *first_k_);
  GotoTable source_gt = GotoTable::Build(*grammar_, cc.Transitions());
  GotoTable dest_gt;

  dest_gt = source_gt;

  TransitionKey key = create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S);
  ASSERT_TRUE(dest_gt.HasGotoState(key));
  EXPECT_EQ(dest_gt.GetGotoState(key), cc.Transitions().at(key));
}

TEST_F(GotoTableTest, MoveAssignment) {
  SetUpSimpleGrammar();
  auto cc = CanonicalCollection::Build(*grammar_, *first_k_);
  GotoTable source_gt = GotoTable::Build(*grammar_, cc.Transitions());
  GotoTable dest_gt;

  dest_gt = std::move(source_gt);

  TransitionKey key = create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S);
  ASSERT_TRUE(dest_gt.HasGotoState(key));
  EXPECT_EQ(dest_gt.GetGotoState(key), cc.Transitions().at(key));
}

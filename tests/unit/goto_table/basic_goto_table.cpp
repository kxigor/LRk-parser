#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <utility>

#include "helpers/CanonicalCollectionTestAccessor.hpp"
#include "lrk_parser/canonical_collection.hpp"
#include "lrk_parser/config.hpp"
#include "lrk_parser/first_k.hpp"
#include "lrk_parser/goto_table.hpp"
#include "lrk_parser/grammar.hpp"

using namespace lrk_parser;
using namespace lrk_parser::details;

class CanonicalCollectionBaseTest : public ::testing::Test {
 protected:
  Grammar grammar_;
  std::unique_ptr<FirstK> first_k_;

  void SetUpSimpleGrammar() {
    StringT T = "a";
    StringT N = "SA";
    CharT Start = 'S';
    VectorT<StringT> rules = {"S->A", "A->a"};
    grammar_ = Grammar::create_from_text(T, N, rules, Start);
    first_k_ = std::make_unique<FirstK>(grammar_, 1);
  }
};

class GotoTableTest : public CanonicalCollectionBaseTest {
 protected:
  TransitionKey create_t_key(StateIdT current_state, CharT symbol) const {
    return TransitionKey{.current_state_id = current_state, .symbol = symbol};
  }
};

const StateIdT TEST_SOURCE_ID = 0;
const CharT TEST_SYMBOL_S = 'S';
const StateIdT TEST_TARGET_ID = 3;

TEST_F(GotoTableTest, DefaultConstructor) {
  GotoTable gt;
  TransitionKey key = create_t_key(0, 'S');
  EXPECT_FALSE(gt.has_goto_state(key));
}

TEST_F(GotoTableTest, CopyConstructorFromCanonicalCollection) {
  SetUpSimpleGrammar();
  CanonicalCollection cc_source(grammar_, *first_k_);

  GotoTable gt(cc_source);

  TransitionKey key = create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S);

  ASSERT_TRUE(gt.has_goto_state(key));
  EXPECT_EQ(gt.get_goto_state(key), TEST_TARGET_ID);
}

TEST_F(GotoTableTest, MoveConstructorFromCanonicalCollection) {
  SetUpSimpleGrammar();

  CanonicalCollection cc_temp(grammar_, *first_k_);
  GotoTable gt(std::move(cc_temp));

  TransitionKey key = create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S);

  ASSERT_TRUE(gt.has_goto_state(key));
  EXPECT_EQ(gt.get_goto_state(key), TEST_TARGET_ID);
}

TEST_F(GotoTableTest, HasGotoStateLogic) {
  SetUpSimpleGrammar();
  CanonicalCollection cc(grammar_, *first_k_);
  GotoTable gt(cc);

  TransitionKey existing_key = create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S);
  EXPECT_TRUE(gt.has_goto_state(existing_key));

  TransitionKey non_existing_key = create_t_key(TEST_TARGET_ID, TEST_SYMBOL_S);
  EXPECT_FALSE(gt.has_goto_state(non_existing_key));

  TransitionKey unused_symbol_key = create_t_key(TEST_SOURCE_ID, 'b');
  EXPECT_FALSE(gt.has_goto_state(unused_symbol_key));
}

TEST_F(GotoTableTest, GetGotoStateLogic) {
  SetUpSimpleGrammar();
  CanonicalCollection cc(grammar_, *first_k_);
  GotoTable gt(cc);

  TransitionKey existing_key = create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S);
  const StateIdT& target_state = gt.get_goto_state(existing_key);

  EXPECT_EQ(target_state, TEST_TARGET_ID);

  TransitionKey non_existing_key = create_t_key(TEST_TARGET_ID, TEST_SYMBOL_S);

  EXPECT_THROW((void)gt.get_goto_state(non_existing_key), std::out_of_range);
}

TEST_F(GotoTableTest, CopyAssignment) {
  SetUpSimpleGrammar();
  CanonicalCollection cc(grammar_, *first_k_);
  GotoTable source_gt(cc);
  GotoTable dest_gt;

  dest_gt = source_gt;

  TransitionKey key = create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S);
  ASSERT_TRUE(dest_gt.has_goto_state(key));
  EXPECT_EQ(dest_gt.get_goto_state(key), TEST_TARGET_ID);
}

TEST_F(GotoTableTest, MoveAssignment) {
  SetUpSimpleGrammar();
  CanonicalCollection cc(grammar_, *first_k_);
  GotoTable source_gt(cc);
  GotoTable dest_gt;

  dest_gt = std::move(source_gt);

  TransitionKey key = create_t_key(TEST_SOURCE_ID, TEST_SYMBOL_S);
  ASSERT_TRUE(dest_gt.has_goto_state(key));
  EXPECT_EQ(dest_gt.get_goto_state(key), TEST_TARGET_ID);
}
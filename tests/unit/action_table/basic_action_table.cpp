#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "lrk_parser/action_table.hpp"
#include "lrk_parser/canonical_collection.hpp"
#include "lrk_parser/config.hpp"
#include "lrk_parser/first_k.hpp"
#include "lrk_parser/grammar.hpp"

using namespace lrk_parser;
using namespace lrk_parser::details;

class ActionTableTest : public ::testing::Test {
 protected:
  Grammar grammar_;
  std::unique_ptr<FirstK> first_k_;
  std::unique_ptr<CanonicalCollection> canonical_collection_;

  void SetUpSimpleGrammar() {
    StringT T = "a";
    StringT N = "SA";
    CharT Start = 'S';
    VectorT<StringT> rules = {"S->A", "A->a"};
    grammar_ = Grammar::create_from_text(T, N, rules, Start);
    first_k_ = std::make_unique<FirstK>(grammar_, 1);
    canonical_collection_ =
        std::make_unique<CanonicalCollection>(grammar_, *first_k_);
  }

  void SetUpConflictGrammar() {
    StringT T = "a";
    StringT N = "S";
    CharT Start = 'S';
    VectorT<StringT> rules = {"S->a", "S->a"};
    grammar_ = Grammar::create_from_text(T, N, rules, Start);
    first_k_ = std::make_unique<FirstK>(grammar_, 1);

    canonical_collection_ =
        std::make_unique<CanonicalCollection>(grammar_, *first_k_);
  }
};

TEST_F(ActionTableTest, BasicShiftReduceAccept) {
  SetUpSimpleGrammar();

  ActionTable action_table(grammar_, *first_k_, *canonical_collection_);

  const auto& goto_table = canonical_collection_->get_goto_table();
  StateIdT state_0 = 0;

  ActionKey shift_key{.state_id = state_0, .lookahead = "a"};

  ASSERT_TRUE(action_table.has_parse_action(shift_key));
  Action shift_act = action_table.get_parse_action(shift_key);

  EXPECT_EQ(shift_act.type, ActionType::Shift);
  TransitionKey goto_key{.current_state_id = state_0, .symbol = 'a'};
  ASSERT_TRUE(goto_table.contains(goto_key));
  EXPECT_EQ(shift_act.value, goto_table.at(goto_key));

  StateIdT state_after_a = goto_table.at(goto_key);

  ActionKey reduce_key{.state_id = state_after_a, .lookahead = ""};

  ASSERT_TRUE(action_table.has_parse_action(reduce_key));
  Action reduce_act = action_table.get_parse_action(reduce_key);

  EXPECT_EQ(reduce_act.type, ActionType::Reduce);
  EXPECT_EQ(reduce_act.value, 2);

  TransitionKey goto_S_key{.current_state_id = state_0, .symbol = 'S'};
  ASSERT_TRUE(goto_table.contains(goto_S_key));
  StateIdT state_after_S = goto_table.at(goto_S_key);

  ActionKey accept_key{.state_id = state_after_S, .lookahead = ""};

  ASSERT_TRUE(action_table.has_parse_action(accept_key));
  Action accept_act = action_table.get_parse_action(accept_key);

  EXPECT_EQ(accept_act.type, ActionType::Accept);
  EXPECT_EQ(accept_act.value, 0);
}

TEST_F(ActionTableTest, DetectsReduceReduceConflict) {
  SetUpConflictGrammar();

  EXPECT_THROW(
      {
        ActionTable action_table(grammar_, *first_k_, *canonical_collection_);
      },
      std::runtime_error);
}

TEST_F(ActionTableTest, DetectsShiftReduceConflict) {
  const auto grammar =
      Grammar::create_from_text("a", "S", {"S->SS", "S->a"}, 'S');
  const FirstK first_k(grammar, 1);
  const CanonicalCollection collection(grammar, first_k);

  EXPECT_THROW((ActionTable{grammar, first_k, collection}), std::runtime_error);
}

TEST_F(ActionTableTest, CopyAndMove) {
  SetUpSimpleGrammar();
  ActionTable src_table(grammar_, *first_k_, *canonical_collection_);

  ActionTable copy_table = src_table;
  ActionKey key{.state_id = 0, .lookahead = "a"};
  EXPECT_TRUE(copy_table.has_parse_action(key));

  ActionTable move_table = std::move(src_table);
  EXPECT_TRUE(move_table.has_parse_action(key));
}

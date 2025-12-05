#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "lrk_parser/canonical_collection.hpp"
#include "lrk_parser/config.hpp"
#include "lrk_parser/first_k.hpp"
#include "lrk_parser/grammar.hpp"
#include "lrk_parser/situation.hpp"

namespace lrk_parser::details {

class CanonicalCollectionTestAccessor : public CanonicalCollection {
 public:
  CanonicalCollectionTestAccessor(const Grammar& grammar, const FirstK& fk)
      : CanonicalCollection(grammar, fk) {}

  details::Situations PublicClosure(const Grammar& grammar,
                                    const FirstK& fist_k,
                                    details::Situations kernel_set) const {
    return closure(grammar, fist_k, std::move(kernel_set));
  }

  details::Situations PublicComputeGo(const Grammar& grammar,
                                      const FirstK& fist_k, std::size_t I_idx,
                                      CharT X) const {
    return compute_go_situation(grammar, fist_k, I_idx, X);
  }

  std::pair<StateIdT, bool> PublicInsertSituations(details::Situations I) {
    return insert_sutiations(std::move(I));
  }

  const StatesT& GetStatesInternal() const { return states_; }
};

}  // namespace lrk_parser::details

using namespace lrk_parser;
using namespace lrk_parser::details;

class CanonicalCollectionTest : public ::testing::Test {
 protected:
  void SetUpSimpleGrammar() {
    StringT T = "a";
    StringT N = "SA";
    CharT Start = 'S';
    VectorT<StringT> rules = {"S->A", "A->a"};
    grammar_ = Grammar::init_with_strs(T, N, rules, Start);
    first_k_ = std::make_unique<FirstK>(grammar_, 1);
  }

  void SetUpRecursiveGrammar() {
    StringT T = "cd";
    StringT N = "SC";
    CharT Start = 'S';
    VectorT<StringT> rules = {"S->CC", "C->cC", "C->d"};
    grammar_ = Grammar::init_with_strs(T, N, rules, Start);
    first_k_ = std::make_unique<FirstK>(grammar_, 1);
  }

  Grammar grammar_;
  std::unique_ptr<FirstK> first_k_;
};

TEST_F(CanonicalCollectionTest, ClosureLogic) {
  SetUpSimpleGrammar();
  const std::size_t S_A_RuleIdx = 1;  // Assuming S->A is index 1

  CanonicalCollectionTestAccessor collection(grammar_, *first_k_);

  Situations kernel;
  kernel.emplace(
      Situation{.rule_idx = S_A_RuleIdx, .dot_pose = 0, .actpref = ""});

  Situations result = collection.PublicClosure(grammar_, *first_k_, kernel);

  EXPECT_EQ(result.size(), 2) << "Closure for [S -> . A] must be 2 situations: "
                                 "the kernel and [A -> . a].";

  bool found_A_rule = false;
  for (const auto& sit : result) {
    const auto& rule = grammar_.get_rule_by_idx(sit.rule_idx);
    if (rule.lhs == 'A' && sit.dot_pose == 0) {
      found_A_rule = true;
    }
  }
  EXPECT_TRUE(found_A_rule)
      << "Closure should add production for non-terminal A";
}

TEST_F(CanonicalCollectionTest, ClosureBranchesEdges) {
  SetUpSimpleGrammar();
  CanonicalCollectionTestAccessor collection(grammar_, *first_k_);

  const std::size_t A_A_RuleIdx = 2;

  Situations kernel;
  kernel.emplace(
      Situation{.rule_idx = A_A_RuleIdx, .dot_pose = 0, .actpref = ""});

  Situations res1 = collection.PublicClosure(grammar_, *first_k_, kernel);
  EXPECT_EQ(res1.size(), 1) << "Test A: Closure on [A -> . a] should only "
                               "contain the kernel (1 situation).";

  Situations kernel_end;
  kernel_end.emplace(
      Situation{.rule_idx = A_A_RuleIdx, .dot_pose = 1, .actpref = ""});

  Situations res2 = collection.PublicClosure(grammar_, *first_k_, kernel_end);
  EXPECT_EQ(res2.size(), 1) << "Test B: Closure on [A -> a .] should only "
                               "contain the kernel (1 situation).";
}

TEST_F(CanonicalCollectionTest, ComputeGoLogic) {
  SetUpSimpleGrammar();
  CanonicalCollectionTestAccessor collection(grammar_, *first_k_);

  const StateIdT I0_id = 0;
  const std::size_t S_A_RuleIdx = 1;

  ASSERT_LT(I0_id, collection.get_states().size());
  ASSERT_FALSE(collection.get_states()[I0_id].empty());

  Situations next_I =
      collection.PublicComputeGo(grammar_, *first_k_, I0_id, 'A');

  EXPECT_EQ(next_I.size(), 1);
  const auto& sit = *next_I.begin();

  EXPECT_EQ(sit.rule_idx, S_A_RuleIdx)
      << "Rule index must be for S -> A (index 1).";
  EXPECT_EQ(sit.dot_pose, 1);
  EXPECT_EQ(sit.actpref, "");

  Situations empty_I =
      collection.PublicComputeGo(grammar_, *first_k_, I0_id, 'b');
  EXPECT_TRUE(empty_I.empty());
}

TEST_F(CanonicalCollectionTest, InsertSituations) {
  SetUpSimpleGrammar();
  CanonicalCollectionTestAccessor collection(grammar_, *first_k_);

  Situations s1;
  s1.emplace(Situation{.rule_idx = 0, .dot_pose = 0, .actpref = "a"});

  auto [id1, inserted1] = collection.PublicInsertSituations(s1);
  EXPECT_TRUE(inserted1);

  auto [id2, inserted2] = collection.PublicInsertSituations(s1);
  EXPECT_FALSE(inserted2);
  EXPECT_EQ(id1, id2);

  EXPECT_EQ(collection.GetStatesInternal().size() - 1, id1);
}

TEST_F(CanonicalCollectionTest, FullBuildRecursiveGrammar) {
  SetUpRecursiveGrammar();

  CanonicalCollection collection(grammar_, *first_k_);

  const auto& goto_table = collection.get_goto_table();
  EXPECT_FALSE(goto_table.empty());

  const auto& states = collection.get_states();

  const auto& s_prime_rules = grammar_.get_rules_idxs(Grammar::kStarSym);
  ASSERT_EQ(s_prime_rules.size(), 1)
      << "Augmented grammar must have exactly one S' rule.";
  auto s_prime_rule_idx = *s_prime_rules.begin();

  bool found_start_trans = false;
  StateIdT next_state_id = 0;

  for (const auto& [key, next_state] : goto_table) {
    if (key.symbol == 'S' && key.current_state_id == 0) {
      found_start_trans = true;
      next_state_id = next_state;
      break;
    }
  }
  EXPECT_TRUE(found_start_trans)
      << "Should have transition by Start symbol 'S' from Initial state I0";

  if (found_start_trans) {
    const auto& target_sits = states[next_state_id];

    EXPECT_EQ(target_sits.size(), 1)
        << "State after GOTO(I0, S) should contain exactly one situation: the "
           "accept situation.";

    const auto& sit = *target_sits.begin();

    EXPECT_EQ(sit.rule_idx, s_prime_rule_idx)
        << "Rule index must be for S' -> S.";
    EXPECT_EQ(sit.dot_pose, 1)
        << "Dot must be at the end of the rule (dot_pose = 1).";
    EXPECT_EQ(sit.actpref, "")
        << "Accept situation lookahead must be empty string (EOF marker).";
  }
}

TEST_F(CanonicalCollectionTest, GettersAndMove) {
  SetUpSimpleGrammar();
  CanonicalCollection collection(grammar_, *first_k_);

  EXPECT_NO_THROW(std::ignore = collection.get_states());
  EXPECT_NO_THROW(std::ignore = collection.get_state_to_id_map());
  EXPECT_FALSE(collection.get_goto_table().empty());

  auto table_copy = collection.get_goto_table();
  auto table_moved = collection.take_goto_table();

  EXPECT_EQ(table_copy, table_moved);

  CanonicalCollection collection2 = collection;
  EXPECT_EQ(collection2.get_states().size(), collection.get_states().size());
}

TEST_F(CanonicalCollectionTest, NoTransitionEdgeCase) {
  StringT T = "ab";
  StringT N = "S";
  CharT Start = 'S';
  VectorT<StringT> rules = {"S->a", "S->b"};
  Grammar g = Grammar::init_with_strs(T, N, rules, Start);
  FirstK fk(g, 1);

  CanonicalCollection collection(g, fk);

  const auto& table = collection.get_goto_table();
  for (const auto& [key, val] : table) {
    const auto& dest_sits = collection.get_states()[val];
    bool is_final = true;
    for (const auto& s : dest_sits) {
      const auto& rhs = g.get_rule_by_idx(s.rule_idx).rhs;
      if (s.dot_pose < rhs.size()) is_final = false;
    }

    if (is_final) {
      for (const auto& [k2, v2] : table) {
        EXPECT_NE(k2.current_state_id, val)
            << "Final state should not have outgoing transitions";
      }
    }
  }
}
#include <gtest/gtest.h>

#include <type_traits>
#include <utility>
#include <variant>

#include "lrk_parser/parser.hpp"
#include "lrk_parser/text_grammar.hpp"

namespace lrk_parser {
namespace {

static_assert(!std::is_default_constructible_v<Parser>);

TEST(Parser, CompilesReadyLR1Parser) {
  const auto grammar =
      ParseGrammar("id=", "E", {"E->id=E", "E->id"}, 'E').value();
  const auto parser = Parser::Compile(grammar, 1);
  ASSERT_TRUE(parser.has_value());
  EXPECT_EQ(parser->Lookahead(), 1);
  EXPECT_TRUE(parser->Accepts("id=id"));
  EXPECT_TRUE(parser->Accepts("id"));
  EXPECT_FALSE(parser->Accepts("id="));
  EXPECT_FALSE(parser->Accepts("id=id="));
}

TEST(Parser, RejectsZeroLookahead) {
  const auto grammar = ParseGrammar("a", "S", {"S->a"}, 'S').value();
  const auto result = Parser::Compile(grammar, 0);
  ASSERT_FALSE(result.has_value());
  ASSERT_TRUE(std::holds_alternative<InvalidLookahead>(result.error()));
  EXPECT_EQ(std::get<InvalidLookahead>(result.error()).k, 0);
}

TEST(Parser, ReportsConflictUntilLookaheadIsSufficient) {
  for (std::size_t required : {2U, 3U}) {
    const auto grammar =
        ParseGrammar("a", "SAB",
                     {"S->A" + StringT(required, 'a'),
                      "S->B" + StringT(required - 1, 'a'), "A->a", "B->a"},
                     'S')
            .value();
    for (std::size_t smaller = 1; smaller < required; ++smaller) {
      const auto result = Parser::Compile(grammar, smaller);
      ASSERT_FALSE(result.has_value());
      ASSERT_TRUE(
          std::holds_alternative<details::ActionConflict>(result.error()));
      EXPECT_EQ(std::get<details::ActionConflict>(result.error()).k, smaller);
    }
    const auto parser = Parser::Compile(grammar, required);
    ASSERT_TRUE(parser.has_value());
    EXPECT_EQ(parser->Lookahead(), required);
    EXPECT_TRUE(parser->Accepts(StringT(required, 'a')));
    EXPECT_TRUE(parser->Accepts(StringT(required + 1, 'a')));
    EXPECT_FALSE(parser->Accepts(StringT(required - 1, 'a')));
  }
}

TEST(Parser, OwnsCompiledDataAndSupportsCopyAndMove) {
  auto compiled = [] {
    const auto grammar = ParseGrammar("a", "S", {"S->aS", "S->"}, 'S').value();
    return Parser::Compile(grammar, 2);
  }();
  ASSERT_TRUE(compiled.has_value());

  const Parser copy = *compiled;
  const Parser moved = std::move(*compiled);
  for (const auto* parser : {&copy, &moved}) {
    EXPECT_EQ(parser->Lookahead(), 2);
    EXPECT_TRUE(parser->Accepts(""));
    EXPECT_TRUE(parser->Accepts("aaa"));
    EXPECT_FALSE(parser->Accepts("aaab"));
  }
}

}  // namespace
}  // namespace lrk_parser

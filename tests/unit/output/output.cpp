#include "lrk_parser/output.hpp"

#include <gtest/gtest.h>

#include <format>
#include <iterator>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>

namespace lrk_parser {
namespace {

template <typename T>
std::string Render(const T& value) {
  std::ostringstream output;
  using details::operator<<;
  output << value;
  return output.str();
}

TEST(Output, SortsUnorderedSetsAndTransitions) {
  const std::unordered_set<StringT> words{"ba", "", "b", "a"};
  EXPECT_EQ(Render(words), "{ε, \"a\", \"b\", \"ba\"}");

  details::TransitionMap transitions;
  transitions.emplace(
      details::TransitionKey{details::StateId{2}, details::EncodeSymbol('z')},
      details::StateId{9});
  transitions.emplace(
      details::TransitionKey{details::StateId{1}, details::EncodeSymbol('b')},
      details::StateId{8});
  transitions.emplace(
      details::TransitionKey{details::StateId{1}, details::EncodeSymbol('a')},
      details::StateId{7});
  EXPECT_EQ(Render(transitions),
            "  (1, a) -> 7\n  (1, b) -> 8\n  (2, z) -> 9\n");
}

TEST(Output, EscapesSymbolsAndPrintsGrammarErrors) {
  const auto grammar =
      MakeGrammar(GrammarSpec{{'b', '\0', static_cast<CharT>(0xff), 'a'},
                              "S",
                              {{'S', StringT{'\0', 'a'}}},
                              'S'})
          .value();
  const auto printed = Render(grammar);
  EXPECT_NE(printed.find("Terminals: {\\x00, a, b, \\xFF}"), std::string::npos);
  EXPECT_NE(printed.find("S -> \\x00a"), std::string::npos);

  const auto invalid = MakeGrammar(GrammarSpec{"a", "S", {{'S', "b"}}, 'S'});
  ASSERT_FALSE(invalid.has_value());
  EXPECT_EQ(Render(invalid.error()),
            "Rule uses an unknown symbol: b (rule #0)");

  const auto syntax = ParseGrammar("a", "S", {"broken"}, 'S');
  ASSERT_FALSE(syntax.has_value());
  EXPECT_EQ(Render(syntax.error()), "Invalid rule syntax (rule #0)");
}

TEST(Output, ParserAndConflictRetainRuleDetailsAfterGrammarDies) {
  const auto parser = [] {
    const auto grammar = ParseGrammar("ab", "S", {"S->a", "S->b"}, 'S').value();
    return Parser::Compile(grammar, 1).value();
  }();
  const auto printed = Render(parser);
  EXPECT_NE(printed.find("#0 <start> -> S"), std::string::npos);
  EXPECT_NE(printed.find("#1 S -> a"), std::string::npos);
  EXPECT_NE(printed.find("#2 S -> b"), std::string::npos);
  const auto a = printed.find("(0, \"a\") = ");
  const auto b = printed.find("(0, \"b\") = ");
  ASSERT_NE(a, std::string::npos);
  ASSERT_NE(b, std::string::npos);
  EXPECT_LT(a, b);

  const auto conflict = [] {
    const auto grammar = ParseGrammar("a", "S", {"S->a", "S->a"}, 'S').value();
    auto compiled = Parser::Compile(grammar, 1);
    EXPECT_FALSE(compiled.has_value());
    return std::move(compiled.error());
  }();
  const auto diagnostic = Render(conflict);
  EXPECT_NE(diagnostic.find("LR(1) conflict"), std::string::npos);
  EXPECT_NE(diagnostic.find("existing: R"), std::string::npos);
  EXPECT_NE(diagnostic.find("incoming: R"), std::string::npos);
  EXPECT_NE(diagnostic.find("(S -> a)"), std::string::npos);
  EXPECT_EQ(Render(CompileError{InvalidLookahead{0}}),
            "Invalid lookahead k=0 (expected k >= 1)");
}

TEST(Output, FormatAndStreamProduceTheSameText) {
  EXPECT_EQ(std::format("{:04}", details::StateId{7}), "0007");
  EXPECT_EQ(std::format("{}", details::kAugmentedStart), "<start>");

  const auto grammar = ParseGrammar("ab", "S", {"S->a", "S->b"}, 'S').value();
  const details::PreparedGrammar prepared{grammar};
  const auto first = details::FirstK::Compute(prepared, 1);
  const auto collection = details::CanonicalCollection::Build(prepared, first);
  const auto actions = details::ActionTable::Build(prepared, first, collection);
  ASSERT_TRUE(actions.has_value());
  const auto gotos =
      details::GotoTable::Build(prepared, collection.Transitions());
  const auto parser = Parser::Compile(grammar, 1);
  ASSERT_TRUE(parser.has_value());

  EXPECT_EQ(std::format("{}", grammar), Render(grammar));
  EXPECT_EQ(std::format("{}", first), Render(first));
  EXPECT_EQ(std::format("{}", collection), Render(collection));
  EXPECT_EQ(std::format("{}", *actions), Render(*actions));
  EXPECT_EQ(std::format("{}", gotos), Render(gotos));
  EXPECT_EQ(std::format("{}", *parser), Render(*parser));

  const CompileError error{InvalidLookahead{0}};
  EXPECT_EQ(std::format("{}", error), Render(error));
  const std::unordered_set<StringT> words{"b", "a"};
  EXPECT_EQ(std::format("{}", output::AsFormatted(words)), Render(words));

  std::string written;
  std::format_to(std::back_inserter(written), "{}", grammar);
  EXPECT_EQ(written, Render(grammar));
}

}  // namespace
}  // namespace lrk_parser

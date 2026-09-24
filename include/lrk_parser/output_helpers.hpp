#pragma once

#include <algorithm>
#include <cstddef>
#include <format>
#include <iosfwd>
#include <sstream>
#include <utility>

#include "action_table.hpp"
#include "canonical_collection.hpp"
#include "config.hpp"
#include "first_k.hpp"
#include "goto_table.hpp"
#include "grammar.hpp"
#include "parser.hpp"
#include "rule.hpp"
#include "situation.hpp"
#include "tables_base.hpp"
#include "text_grammar.hpp"

namespace lrk_parser {
std::ostream& operator<<(std::ostream& os, const Rule& rule);
std::ostream& operator<<(std::ostream& os, const Grammar& grammar);
std::ostream& operator<<(std::ostream& os, const GrammarError& error);
std::ostream& operator<<(std::ostream& os, const RuleSyntaxError& error);
std::ostream& operator<<(std::ostream& os, const TextGrammarError& error);
namespace details {
std::ostream& operator<<(std::ostream& os, const PreparedRule& rule);
std::ostream& operator<<(std::ostream& os, const PreparedGrammar& grammar);
std::ostream& operator<<(std::ostream& os, const Situation& sit);
std::ostream& operator<<(std::ostream& os, const Situations& sits);
std::ostream& operator<<(std::ostream& os, const UsetT<StringT>& set);
std::ostream& operator<<(std::ostream& os, const FirstK& first_k_obj);
std::ostream& operator<<(std::ostream& os, const TransitionKey& tkey);
std::ostream& operator<<(std::ostream& os, const TransitionMap& transitions);
std::ostream& operator<<(std::ostream& os, const CanonicalCollection& cc);
std::ostream& operator<<(std::ostream& os, const Action& action);
std::ostream& operator<<(std::ostream& os, const ActionKey& a_key);
std::ostream& operator<<(std::ostream& os, const GotoTable& table);
std::ostream& operator<<(std::ostream& os, const ActionTable& table);
std::ostream& operator<<(std::ostream& os, const ActionConflict& error);
}  // namespace details
std::ostream& operator<<(std::ostream& os, const InvalidLookahead& error);
std::ostream& operator<<(std::ostream& os, const CompileError& error);
std::ostream& operator<<(std::ostream& os, const Parser& parser);
}  // namespace lrk_parser

namespace lrk_parser::output {

template <typename T>
struct OstreamFormatter {
  constexpr auto parse(std::format_parse_context& context) {
    return context.begin();
  }

  auto format(const T& value, std::format_context& context) const {
    std::ostringstream output;
    using details::operator<<;
    output << value;
    const auto rendered = output.str();
    return std::copy(rendered.begin(), rendered.end(), context.out());
  }
};

}  // namespace lrk_parser::output

template <>
struct std::formatter<lrk_parser::details::StateId>
    : std::formatter<std::size_t> {
  auto format(lrk_parser::details::StateId id,
              std::format_context& context) const {
    return std::formatter<std::size_t>::format(std::to_underlying(id), context);
  }
};

template <>
struct std::formatter<lrk_parser::details::RuleId>
    : std::formatter<std::size_t> {
  auto format(lrk_parser::details::RuleId id,
              std::format_context& context) const {
    return std::formatter<std::size_t>::format(std::to_underlying(id), context);
  }
};

template <>
struct std::formatter<lrk_parser::details::SymbolId> {
  constexpr auto parse(std::format_parse_context& context) {
    return context.begin();
  }

  auto format(lrk_parser::details::SymbolId id,
              std::format_context& context) const {
    if (id == lrk_parser::details::kAugmentedStart) {
      return std::format_to(context.out(), "<start>");
    }
    return std::format_to(context.out(), "{}",
                          lrk_parser::details::DecodeSymbol(id));
  }
};

template <>
struct std::formatter<lrk_parser::Rule>
    : lrk_parser::output::OstreamFormatter<lrk_parser::Rule> {};

template <>
struct std::formatter<lrk_parser::Grammar>
    : lrk_parser::output::OstreamFormatter<lrk_parser::Grammar> {};

template <>
struct std::formatter<lrk_parser::GrammarError>
    : lrk_parser::output::OstreamFormatter<lrk_parser::GrammarError> {};

template <>
struct std::formatter<lrk_parser::RuleSyntaxError>
    : lrk_parser::output::OstreamFormatter<lrk_parser::RuleSyntaxError> {};

template <>
struct std::formatter<lrk_parser::TextGrammarError>
    : lrk_parser::output::OstreamFormatter<lrk_parser::TextGrammarError> {};

template <>
struct std::formatter<lrk_parser::details::PreparedRule>
    : lrk_parser::output::OstreamFormatter<lrk_parser::details::PreparedRule> {
};

template <>
struct std::formatter<lrk_parser::details::PreparedGrammar>
    : lrk_parser::output::OstreamFormatter<
          lrk_parser::details::PreparedGrammar> {};

template <>
struct std::formatter<lrk_parser::details::Situation>
    : lrk_parser::output::OstreamFormatter<lrk_parser::details::Situation> {};

template <>
struct std::formatter<lrk_parser::details::Situations>
    : lrk_parser::output::OstreamFormatter<lrk_parser::details::Situations> {};

template <>
struct std::formatter<lrk_parser::UsetT<lrk_parser::StringT>>
    : lrk_parser::output::OstreamFormatter<
          lrk_parser::UsetT<lrk_parser::StringT>> {};

template <>
struct std::formatter<lrk_parser::details::FirstK>
    : lrk_parser::output::OstreamFormatter<lrk_parser::details::FirstK> {};

template <>
struct std::formatter<lrk_parser::details::TransitionKey>
    : lrk_parser::output::OstreamFormatter<lrk_parser::details::TransitionKey> {
};

template <>
struct std::formatter<lrk_parser::details::TransitionMap>
    : lrk_parser::output::OstreamFormatter<lrk_parser::details::TransitionMap> {
};

template <>
struct std::formatter<lrk_parser::details::CanonicalCollection>
    : lrk_parser::output::OstreamFormatter<
          lrk_parser::details::CanonicalCollection> {};

template <>
struct std::formatter<lrk_parser::details::Action>
    : lrk_parser::output::OstreamFormatter<lrk_parser::details::Action> {};

template <>
struct std::formatter<lrk_parser::details::ActionKey>
    : lrk_parser::output::OstreamFormatter<lrk_parser::details::ActionKey> {};

template <>
struct std::formatter<lrk_parser::details::ActionTable>
    : lrk_parser::output::OstreamFormatter<lrk_parser::details::ActionTable> {};

template <>
struct std::formatter<lrk_parser::details::GotoTable>
    : lrk_parser::output::OstreamFormatter<lrk_parser::details::GotoTable> {};

template <>
struct std::formatter<lrk_parser::details::ActionConflict>
    : lrk_parser::output::OstreamFormatter<
          lrk_parser::details::ActionConflict> {};

template <>
struct std::formatter<lrk_parser::InvalidLookahead>
    : lrk_parser::output::OstreamFormatter<lrk_parser::InvalidLookahead> {};

template <>
struct std::formatter<lrk_parser::CompileError>
    : lrk_parser::output::OstreamFormatter<lrk_parser::CompileError> {};

template <>
struct std::formatter<lrk_parser::Parser>
    : lrk_parser::output::OstreamFormatter<lrk_parser::Parser> {};

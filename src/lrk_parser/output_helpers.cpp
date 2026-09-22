#include "lrk_parser/output_helpers.hpp"

#include <algorithm>
#include <cstddef>
#include <format>
#include <ostream>
#include <ranges>
#include <utility>

#include "lrk_parser/action_table.hpp"
#include "lrk_parser/canonical_collection.hpp"
#include "lrk_parser/config.hpp"
#include "lrk_parser/first_k.hpp"
#include "lrk_parser/goto_table.hpp"
#include "lrk_parser/grammar.hpp"
#include "lrk_parser/parser.hpp"
#include "lrk_parser/rule.hpp"
#include "lrk_parser/situation.hpp"
#include "lrk_parser/tables_base.hpp"

using CharT = lrk_parser::CharT;
using Rule = lrk_parser::Rule;
using StringT = lrk_parser::StringT;
using Grammar = lrk_parser::Grammar;
using StateId = lrk_parser::details::StateId;
using BaseGotoTableT = lrk_parser::details::CanonicalCollection::BaseGotoTableT;

/*TODO: доработать консольный вывыод, он кривой*/

template <>
struct std::formatter<lrk_parser::details::StateId>
    : std::formatter<std::size_t> {
  auto format(lrk_parser::details::StateId id, std::format_context& ctx) const {
    return std::formatter<std::size_t>::format(std::to_underlying(id), ctx);
  }
};

template <>
struct std::formatter<lrk_parser::details::RuleId>
    : std::formatter<std::size_t> {
  auto format(lrk_parser::details::RuleId id, std::format_context& ctx) const {
    return std::formatter<std::size_t>::format(std::to_underlying(id), ctx);
  }
};

template <>
struct std::formatter<lrk_parser::details::SymbolId> {
  static constexpr auto parse(std::format_parse_context& ctx) {
    return ctx.begin();
  }

  static auto format(lrk_parser::details::SymbolId id,
                     std::format_context& ctx) {
    if (id == lrk_parser::details::kAugmentedStart) {
      return std::format_to(ctx.out(), "<start>");
    }
    return std::format_to(ctx.out(), "{}",
                          lrk_parser::details::DecodeSymbol(id));
  }
};

template <>
struct std::formatter<lrk_parser::Rule> {
  static constexpr auto parse(std::format_parse_context& ctx) {
    return std::ranges::find(ctx.begin(), ctx.end(), '}');
  }

  static auto format(const lrk_parser::Rule& rule, std::format_context& ctx) {
    auto out = ctx.out();

    out = std::format_to(out, "{} {} ", rule.lhs, "->");

    if (rule.rhs.empty()) {
      out = std::format_to(out, "ε");
    } else {
      out = std::format_to(out, "{}", rule.rhs);
    }

    return out;
  }
};

template <>
struct std::formatter<lrk_parser::UsetT<StringT>> {
  static constexpr auto parse(std::format_parse_context& ctx) {
    return std::ranges::find(ctx.begin(), ctx.end(), '}');
  }

  static auto format(const lrk_parser::UsetT<StringT>& set,
                     std::format_context& ctx) {
    auto out = ctx.out();
    out = std::format_to(out, "{{");

    bool first = true;
    for (const auto& item : set) {
      if (not first) {
        out = std::format_to(out, ", ");
      }
      if (item.empty()) {
        out = std::format_to(out, "ε");
      } else {
        out = std::format_to(out, "\"{}\"", item);
      }
      first = false;
    }
    out = std::format_to(out, "}}");

    return out;
  }
};

template <>
struct std::formatter<lrk_parser::details::FirstK> {
  static constexpr auto parse(std::format_parse_context& ctx) {
    return std::ranges::find(ctx.begin(), ctx.end(), '}');
  }

  static auto format(const lrk_parser::details::FirstK& first_k_obj,
                     std::format_context& ctx) {
    auto out = ctx.out();

    out = std::format_to(
        out,
        "--------------------- First-K Sets (K = {}) ---------------------\n",
        first_k_obj.k_);

    for (const auto& pair : first_k_obj.first_k_) {
      const auto& symbol = pair.first;
      const auto& first_k_set = pair.second;

      out = std::format_to(out, "  First_{}({}) = {}\n", first_k_obj.k_, symbol,
                           first_k_set);
    }

    out = std::format_to(
        out,
        "----------------------------------------------------------------\n");

    return out;
  }
};

template <>
struct std::formatter<lrk_parser::UsetT<lrk_parser::CharT>> {
  static constexpr auto parse(std::format_parse_context& ctx) {
    return std::ranges::find(ctx.begin(), ctx.end(), '}');
  }

  static auto format(const lrk_parser::UsetT<lrk_parser::CharT>& set,
                     std::format_context& ctx) {
    auto out = ctx.out();
    out = std::format_to(out, "{{");
    if (not set.empty()) {
      out = std::format_to(out, "{}", *set.begin());
    }
    for (const auto& sym : set | std::views::drop(1)) {
      out = std::format_to(out, ", {}", sym);
    }
    out = std::format_to(out, "}}");
    return out;
  }
};

template <>
struct std::formatter<lrk_parser::Grammar> {
  static constexpr auto parse(std::format_parse_context& ctx) {
    return ctx.begin();
  }

  static auto format(const lrk_parser::Grammar& grammar,
                     std::format_context& ctx) {
    auto out = std::format_to(
        ctx.out(),
        "--- LR(k) Grammar ---\nNon-Terminals (N) = {}\nTerminals (T) = {}\n"
        "Start Symbol (S) = {}\nProduction Rules (P):\n",
        grammar.Nonterminals(), grammar.Terminals(), grammar.Start());
    for (const auto& rule : grammar.Rules()) {
      out = std::format_to(out, "  {}\n", rule);
    }
    return out;
  }
};

template <>
struct std::formatter<lrk_parser::details::PreparedGrammar> {
  static constexpr auto parse(std::format_parse_context& ctx) {
    return ctx.begin();
  }

  static auto format(const lrk_parser::details::PreparedGrammar& grammar,
                     std::format_context& ctx) {
    auto out = std::format_to(ctx.out(), "Prepared production rules:\n");
    for (const auto& rule : grammar.Rules()) {
      out = std::format_to(out, "  {} -> ", rule.lhs);
      if (rule.rhs.empty()) out = std::format_to(out, "ε");
      for (auto symbol : rule.rhs) out = std::format_to(out, "{}", symbol);
      out = std::format_to(out, "\n");
    }
    return out;
  }
};

template <>
struct std::formatter<lrk_parser::details::Situation> {
  static constexpr auto parse(std::format_parse_context& ctx) {
    return std::ranges::find(ctx.begin(), ctx.end(), '}');
  }

  static auto format(const lrk_parser::details::Situation& sit,
                     std::format_context& ctx) {
    auto out = ctx.out();

    out = std::format_to(out, "[Rule={}, Dot={}, Lookahead=\"", sit.rule_idx,
                         sit.dot_pose);

    if (sit.actpref.empty()) {
      out = std::format_to(out, "ε");
    } else {
      out = std::format_to(out, "{}", sit.actpref);
    }

    out = std::format_to(out, "\"]");

    return out;
  }
};

template <>
struct std::formatter<lrk_parser::details::Situations> {
  static constexpr auto parse(std::format_parse_context& ctx) {
    return std::ranges::find(ctx.begin(), ctx.end(), '}');
  }

  static auto format(const lrk_parser::details::Situations& sits,
                     std::format_context& ctx) {
    auto out = ctx.out();

    out = std::format_to(out, "{{\n");

    if (not sits.empty()) {
      out = std::format_to(out, "    {}", *sits.begin());
    }

    for (const auto& sit : sits | std::views::drop(1)) {
      out = std::format_to(out, ",\n");
      out = std::format_to(out, "    {}", sit);
    }

    if (not sits.empty()) {
      out = std::format_to(out, "\n");
    }

    out = std::format_to(out, "}}");

    return out;
  }
};

template <>
struct std::formatter<lrk_parser::details::TransitionKey> {
  static constexpr auto parse(std::format_parse_context& ctx) {
    return std::ranges::find(ctx.begin(), ctx.end(), '}');
  }

  static auto format(const lrk_parser::details::TransitionKey& tkey,
                     std::format_context& ctx) {
    return std::format_to(ctx.out(), "({}, {})", tkey.current_state_id,
                          tkey.symbol);
  }
};

template <>
struct std::formatter<BaseGotoTableT> {
  static constexpr auto parse(std::format_parse_context& ctx) {
    return std::ranges::find(ctx.begin(), ctx.end(), '}');
  }

  static auto format(const BaseGotoTableT& table_map,
                     std::format_context& ctx) {
    auto out = ctx.out();

    if (table_map.empty()) {
      return std::format_to(out, "  (Empty GOTO table)\n");
    }

    lrk_parser::UmapT<
        StateId,
        lrk_parser::VectorT<std::pair<lrk_parser::details::SymbolId, StateId>>>
        sorted_transitions;
    for (const auto& pair : table_map) {
      const auto& tkey = pair.first;
      const auto& next_state_id = pair.second;
      sorted_transitions[tkey.current_state_id].emplace_back(tkey.symbol,
                                                             next_state_id);
    }

    for (const auto& state_pair : sorted_transitions) {
      const auto current_state_id = state_pair.first;
      const auto& transitions = state_pair.second;

      for (const auto& transition : transitions) {
        out = std::format_to(out, "  GOTO({}, {}) = {}\n", current_state_id,
                             transition.first, transition.second);
      }
    }

    return out;
  }
};

template <>
struct std::formatter<lrk_parser::details::CanonicalCollection> {
  static constexpr auto parse(std::format_parse_context& ctx) {
    return std::ranges::find(ctx.begin(), ctx.end(), '}');
  }

  static auto format(const lrk_parser::details::CanonicalCollection& cc,
                     std::format_context& ctx) {
    auto out = ctx.out();

    out = std::format_to(
        out, "-------------- Canonical LR(K) Collection --------------\n");

    out = std::format_to(out, "\n## States:\n");

    const auto& states = cc.get_states();
    for (std::size_t i = 0; i < states.size(); ++i) {
      out = std::format_to(out, "  State {}:\n", i);
      out = std::format_to(out, "{}\n", states[i]);
    }

    out = std::format_to(out, "\n## Goto Table:\n");
    out = std::format_to(out, "{}", cc.get_goto_table());

    out = std::format_to(
        out,
        "----------------------------------------------------------------\n");

    return out;
  }
};

template <>
struct std::formatter<lrk_parser::details::ActionType> {
  static constexpr auto parse(std::format_parse_context& ctx) {
    return std::ranges::find(ctx.begin(), ctx.end(), '}');
  }

  static auto format(const lrk_parser::details::ActionType& type,
                     std::format_context& ctx) {
    switch (type) {
      case lrk_parser::details::ActionType::Shift:
        return std::format_to(ctx.out(), "Shift");
      case lrk_parser::details::ActionType::Reduce:
        return std::format_to(ctx.out(), "Reduce");
      case lrk_parser::details::ActionType::Accept:
        return std::format_to(ctx.out(), "Accept");
      case lrk_parser::details::ActionType::Error:
      default:
        return std::format_to(ctx.out(), "Error");
    }
    std::unreachable();
  }
};

template <>
struct std::formatter<lrk_parser::details::Action> {
  static constexpr auto parse(std::format_parse_context& ctx) {
    return std::ranges::find(ctx.begin(), ctx.end(), '}');
  }

  static auto format(const lrk_parser::details::Action& action,
                     std::format_context& ctx) {
    switch (action.type) {
      case lrk_parser::details::ActionType::Shift:
        return std::format_to(ctx.out(), "S{}", action.value);
      case lrk_parser::details::ActionType::Reduce:
        return std::format_to(ctx.out(), "R{}", action.value);
      case lrk_parser::details::ActionType::Accept:
        return std::format_to(ctx.out(), "ACC");
      case lrk_parser::details::ActionType::Error:
      default:
        return std::format_to(ctx.out(), "ERR");
    }
    std::unreachable();
  }
};

template <>
struct std::formatter<lrk_parser::details::ActionKey> {
  static constexpr auto parse(std::format_parse_context& ctx) {
    return std::ranges::find(ctx.begin(), ctx.end(), '}');
  }

  static auto format(const lrk_parser::details::ActionKey& a_key,
                     std::format_context& ctx) {
    auto out = std::format_to(ctx.out(), "({}, \"", a_key.state_id);

    if (a_key.lookahead.empty()) {
      out = std::format_to(out, "ε");
    } else {
      out = std::format_to(out, "{}", a_key.lookahead);
    }

    out = std::format_to(out, "\")");

    return out;
  }
};

// NOLINTBEGIN
template <>
struct std::formatter<lrk_parser::details::ActionTable> {
  static constexpr auto parse(std::format_parse_context& ctx) {
    return std::ranges::find(ctx.begin(), ctx.end(), '}');
  }

  static auto format(const lrk_parser::details::ActionTable& table,
                     std::format_context& ctx) {
    static constexpr const std::size_t TABLE_LINE_WIDTH = 64;
    static constexpr const int COLUMN_WIDTH = 10;
    auto out = ctx.out();
    const auto& action_map = table.action_table_;

    out = std::format_to(out, "{:-^{}}\n", " LR(K) Action Table ",
                         TABLE_LINE_WIDTH);
    if (action_map.empty()) {
      out = std::format_to(out, "  (Table is empty)\n");
      out = std::format_to(out, "{:-<{}}\n", "", TABLE_LINE_WIDTH);
      return out;
    }

    lrk_parser::UsetT<StateId> unique_states;
    lrk_parser::UsetT<StringT> unique_lookaheads;
    for (const auto& pair : action_map) {
      unique_states.insert(pair.first.state_id);
      unique_lookaheads.insert(pair.first.lookahead);
    }

    out = std::format_to(out, "{:<{}}", "State", COLUMN_WIDTH);
    for (const auto& lookahead : unique_lookaheads) {
      if (not lookahead.empty()) {
        out = std::format_to(out, "{:^{}}", lookahead, COLUMN_WIDTH);
      } else {
        out = std::format_to(out, "{:^{}}", "ε", COLUMN_WIDTH);
      }
    }
    out = std::format_to(out, "\n");

    const auto kTotalCols = unique_lookaheads.size() + 1;
    const auto kLineLength = COLUMN_WIDTH * kTotalCols;
    out = std::format_to(out, "{:-<{}}\n", "", kLineLength);

    for (const auto& state_id : unique_states) {
      out = std::format_to(out, "{:<{}}", state_id, COLUMN_WIDTH);

      for (const auto& lookahead : unique_lookaheads) {
        const lrk_parser::details::ActionKey kAKey = {.state_id = state_id,
                                                      .lookahead = lookahead};

        std::array<char, COLUMN_WIDTH> buffer;
        std::size_t content_len = 0;

        if (table.has_parse_action(kAKey)) {
          auto result = std::format_to_n(buffer.data(), buffer.size(), "{}",
                                         table.get_parse_action(kAKey));
          content_len =
              std::min(static_cast<std::size_t>(result.size), buffer.size());
        }

        int total_padding = COLUMN_WIDTH - static_cast<int>(content_len);
        if (total_padding < 0) total_padding = 0;

        int left_pad = total_padding / 2;
        int right_pad = total_padding - left_pad;

        out = std::format_to(out, "{:{}}", "", left_pad);

        out = std::copy_n(buffer.data(), content_len, out);

        out = std::format_to(out, "{:{}}", "", right_pad);
      }
      out = std::format_to(out, "\n");
    }
    out = std::format_to(out, "{:-<{}}\n", "", TABLE_LINE_WIDTH);
    return out;
  }
};
// NOLINTEND

template <>
struct std::formatter<lrk_parser::details::GotoTable> {
  static constexpr auto parse(std::format_parse_context& ctx) {
    return std::ranges::find(ctx.begin(), ctx.end(), '}');
  }

  static auto format(const lrk_parser::details::GotoTable& table,
                     std::format_context& ctx) {
    return std::format_to(ctx.out(), "{}", table.goto_table_);
  }
};

template <>
struct std::formatter<lrk_parser::LrkParser> {
  static constexpr auto parse(std::format_parse_context& ctx) {
    return std::ranges::find(ctx.begin(), ctx.end(), '}');
  }

  static auto format(const lrk_parser::LrkParser& parser,
                     std::format_context& ctx) {
    static const auto kLineWidth = 64;
    auto out = ctx.out();

    out = std::format_to(out, "{:-^{}}\n", " LR(K) Parser Configuration ",
                         kLineWidth);
    out = std::format_to(out, "Parsing Lookahead (K) = {}\n\n", parser.k_);
    if (parser.grammar_) out = std::format_to(out, "{}\n", *parser.grammar_);
    out = std::format_to(out, "{}\n", parser.goto_table_);
    out = std::format_to(out, "{}\n", parser.action_table_);
    out = std::format_to(out, "{:-<{}}\n", "", kLineWidth);
    return out;
  }
};

namespace lrk_parser {

std::ostream& operator<<(std::ostream& os, const Grammar& grammar) {
  return os << std::format("{}", grammar);
}

std::ostream& operator<<(std::ostream& os, const Rule& rule) {
  return os << std::format("{}", rule);
}

namespace details {

std::ostream& operator<<(std::ostream& os, const UsetT<StringT>& set) {
  return os << std::format("{}", set);
}

std::ostream& operator<<(std::ostream& os, const FirstK& first_k_obj) {
  return os << std::format("{}", first_k_obj);
}

std::ostream& operator<<(std::ostream& os, const ActionType& type) {
  return os << std::format("{}", type);
}

std::ostream& operator<<(std::ostream& os, const CanonicalCollection& cc) {
  return os << std::format("{}", cc);
}

std::ostream& operator<<(std::ostream& os, const Action& action) {
  return os << std::format("{}", action);
}

std::ostream& operator<<(std::ostream& os, const ActionTable& table) {
  return os << std::format("{}", table);
}

std::ostream& operator<<(std::ostream& os, const ActionKey& a_key) {
  return os << std::format("{}", a_key);
}

std::ostream& operator<<(std::ostream& os, const GotoTable& table) {
  return os << std::format("{}", table);
}

std::ostream& operator<<(std::ostream& os, const Situation& sit) {
  return os << std::format("{}", sit);
}

std::ostream& operator<<(std::ostream& os, const Situations& sits) {
  return os << std::format("{}", sits);
}

std::ostream& operator<<(std::ostream& os, const TransitionKey& tkey) {
  return os << std::format("{}", tkey);
}
}  // namespace details

std::ostream& operator<<(std::ostream& os, const LrkParser& parser) {
  return os << std::format("{}", parser);
}

}  // namespace lrk_parser
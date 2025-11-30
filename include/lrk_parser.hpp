#pragma once

#include <algorithm>
#include <cstddef>
#include <format>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace lrk_parser {
class LrkParser {
  /*====================== Usings/Helpers ======================*/
  using CharT = char;
  using StringT = std::basic_string<CharT>;
  using StringViewT = std::basic_string_view<CharT>;

  template <typename T>
  using VectorT = std::vector<T>;

  template <typename T, typename H = std::hash<T>>
  using UsetT = std::unordered_set<T, H>;

  template <typename T, typename U, typename H = std::hash<T>>
  using UmapT = std::unordered_map<T, U, H>;

  struct Situation {
    std::size_t rule_idx{};
    std::size_t dot_pose{};
    std::size_t lookahead_idx{};
  };

  struct SituationHash {
    /*=========== Hash ===========*/
    [[nodiscard]] std::size_t operator()(const Situation& sit) const noexcept;
  };

  using Situations = UsetT<Situation, SituationHash>;

  struct Rule {
    CharT lhs{};
    StringT rhs;
  };

  struct Grammar {
    /*======== Constants =========*/
    static constexpr const CharT kStartChar = CharT{'@'};
    static constexpr const StringViewT kArrow = "->";
    static constexpr const std::size_t kArrowPos = 1;
    static constexpr const std::size_t kMinSize = 1 + kArrow.size();

    /*========= Factory ==========*/
    static Grammar init_with_strs(StringT terminals, StringT nonterminals,
                                  VectorT<StringT> rules_str, CharT start) {
      prepare_rules_str(rules_str);

      Grammar grammar = {
          .terminals{terminals.begin(), terminals.end()},
          .nonterminals{nonterminals.begin(), nonterminals.end()}};

      grammar.throw_if_wrong_terminal_nontermianls();

      grammar.add_rule(kStartChar, {start});

      for (const auto& rule_str : rules_str) {
        grammar.add_rule(rule_from_str(rule_str));
      }
      return grammar;
    }

    /*========== Impls ===========*/
    static Rule rule_from_str(const StringT& rule_str) {
      return Rule{.lhs = rule_str[0],
                  .rhs = {rule_str.begin() + kMinSize, rule_str.end()}};
    }

    void add_rule(CharT lhs, StringT rhs) {
      add_rule(Rule{.lhs = std::move(lhs), .rhs = std::move(rhs)});
    }

    void add_rule(Rule rule) {
      const std::size_t kNewRuleIdx = rules.size();
      lhs_to_rule_idxs.emplace(rule.lhs, kNewRuleIdx);
      rules.emplace_back(std::move(rule));
    }

    void throw_if_wrong_terminal_nontermianls() const {
      if (terminals.contains(kStartChar) or nonterminals.contains(kStartChar)) {
        throw std::logic_error(
            "the @ symbol is reserved by the grammar, it cannot be used");
      }

      for (const auto& terminal : terminals) {
        if (nonterminals.contains(terminal)) {
          throw std::logic_error(
              "the set of terminal and non-terminal symbols cannot overlap");
        }
      }
    }

    void throw_if_wrong_rule_str(const StringT& rule_str) const {
      /*TODO: improve code*/

      if (rule_str.size() < kMinSize) {
        throw std::logic_error("str rule requires a size of at least");
      }
      if (auto find_res = rule_str.find(kArrow); find_res != kArrowPos) {
        throw std::logic_error("arrow missing or in the wrong place");
      }
      const auto& lhs_sym = rule_str[0];

      if (terminals.contains(lhs_sym)) {
        throw std::logic_error(
            "there can't be a terminal on the left of the rule");
      }

      for (std::size_t i = 0; i < kArrowPos; ++i) {
        if (not terminals.contains(rule_str[i]) and
            not nonterminals.contains(rule_str[i])) {
          throw std::logic_error("an unknown symbol has been encountered");
        }
      }

      for (std::size_t i = kArrowPos + kArrow.size(); i < rule_str.size();
           ++i) {
        if (not terminals.contains(rule_str[i]) and
            not nonterminals.contains(rule_str[i])) {
          throw std::logic_error("an unknown symbol has been encountered");
        }
      }
    }

    static void prepare_rules_str(VectorT<StringT>& rules_str) {
      for (auto& rule_str : rules_str) {
        std::ranges::remove_if(rule_str,
                               [](auto& sym) { return std::isspace(sym); });
      }
    }

    /*======= Data fields ========*/
    UsetT<CharT> terminals;
    UsetT<CharT> nonterminals;
    VectorT<Rule> rules;
    UmapT<CharT, VectorT<std::size_t>> lhs_to_rule_idxs;
  };
};
}  // namespace lrk_parser
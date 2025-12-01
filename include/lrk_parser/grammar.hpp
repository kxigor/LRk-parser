#pragma once

#include <algorithm>
#include <ranges>

#include "config.hpp"
#include "rule.hpp"

namespace lrk_parser {

struct Grammar {
  /*======== Constants =========*/
  static constexpr const CharT kStarSym = CharT{'@'};
  static constexpr const StringViewT kArrow = "->";
  static constexpr const std::size_t kArrowPos = 1;
  static constexpr const std::size_t kMinSize = 1 + kArrow.size();

  /*========= Factory ==========*/
  static Grammar init_with_strs(StringT terminals, StringT nonterminals,
                                VectorT<StringT> rules_str, CharT start) {
    prepare_rules_str(rules_str);

    Grammar grammar = {.terminals{terminals.begin(), terminals.end()},
                       .nonterminals{nonterminals.begin(), nonterminals.end()}};

    grammar.throw_if_wrong_terminal_nontermianls();

    grammar.add_rule(kStarSym, {start});

    for (const auto& rule_str : rules_str) {
      grammar.add_rule(rule_from_str(rule_str));
    }
    return grammar;
  }

  /*== Symbol classification ===*/
  [[nodiscard]] bool is_terminal(CharT sym) const noexcept {
    return terminals.contains(sym);
  }

  [[nodiscard]] bool is_nonterminal(CharT sym) const noexcept {
    return nonterminals.contains(sym);
  }

  /*======= Rule lookup ========*/
  [[nodiscard]] bool is_rules_exists(CharT sym) const noexcept {
    return lhs_to_rule_idxs.contains(sym);
  }

  [[nodiscard]] const VectorT<std::size_t>& get_rules_idxs(
      CharT sym) const noexcept {
    return lhs_to_rule_idxs.at(sym);
  }

  [[nodiscard]] const Rule& get_rule_by_idx(
      std::size_t rule_idx) const noexcept {
    return rules[rule_idx];
  }

  /*========== Impls ===========*/
  [[nodiscard]] auto get_all_symbols_range() {
    return all_sets | std::views::transform([](auto s) -> const auto& {
             return s.get();
           }) |
           std::views::join;
  }

 private:
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
    if (terminals.contains(kStarSym) or nonterminals.contains(kStarSym)) {
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

    for (std::size_t i = kArrowPos + kArrow.size(); i < rule_str.size(); ++i) {
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
 public:
  UsetT<CharT> terminals;
  UsetT<CharT> nonterminals;
  VectorT<Rule> rules;
  UmapT<CharT, VectorT<std::size_t>> lhs_to_rule_idxs;

  VectorT<std::reference_wrapper<const UsetT<CharT>>> all_sets{
      std::cref(terminals), std::cref(nonterminals)};
};
}  // namespace lrk_parser
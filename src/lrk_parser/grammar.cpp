#include "lrk_parser/grammar.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>

#include "lrk_parser/config.hpp"
#include "lrk_parser/rule.hpp"

lrk_parser::Grammar lrk_parser::Grammar::init_with_strs(
    StringT terminals, StringT nonterminals, VectorT<StringT> rules_str,
    CharT start) {
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

bool lrk_parser::Grammar::is_terminal(CharT sym) const noexcept {
  return terminals.contains(sym);
}

bool lrk_parser::Grammar::is_nonterminal(CharT sym) const noexcept {
  return nonterminals.contains(sym);
}

bool lrk_parser::Grammar::is_rules_exists(CharT sym) const noexcept {
  return lhs_to_rule_idxs.contains(sym);
}

const lrk_parser::VectorT<std::size_t>& lrk_parser::Grammar::get_rules_idxs(
    CharT sym) const noexcept {
  return lhs_to_rule_idxs.at(sym);
}

const lrk_parser::details::Rule& lrk_parser::Grammar::get_rule_by_idx(
    std::size_t rule_idx) const noexcept {
  return rules[rule_idx];
}

lrk_parser::details::Rule lrk_parser::Grammar::rule_from_str(
    const StringT& rule_str) {
  return details::Rule{.lhs = rule_str[0],
                       .rhs = {rule_str.begin() + kMinSize, rule_str.end()}};
}

void lrk_parser::Grammar::add_rule(CharT lhs, StringT rhs) {
  add_rule(details::Rule{.lhs = std::move(lhs), .rhs = std::move(rhs)});
}

void lrk_parser::Grammar::add_rule(details::Rule rule) {
  const std::size_t kNewRuleIdx = rules.size();
  lhs_to_rule_idxs.emplace(rule.lhs, kNewRuleIdx);
  rules.emplace_back(std::move(rule));
}

void lrk_parser::Grammar::throw_if_wrong_terminal_nontermianls() const {
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

void lrk_parser::Grammar::throw_if_wrong_rule_str(
    const StringT& rule_str) const {
  if (rule_str.size() < kMinSize) {
    throw std::logic_error("str rule requires a size of at least");
  }
  if (auto find_res = rule_str.find(kArrow); find_res != kArrowPos) {
    throw std::logic_error("arrow missing or in the wrong place");
  }
  const auto& lhs_sym = rule_str[0];

  if (terminals.contains(lhs_sym)) {
    throw std::logic_error("there can't be a terminal on the left of the rule");
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

void lrk_parser::Grammar::prepare_rules_str(VectorT<StringT>& rules_str) {
  for (auto& rule_str : rules_str) {
    std::ranges::remove_if(rule_str,
                           [](auto& sym) { return std::isspace(sym); });
  }
}
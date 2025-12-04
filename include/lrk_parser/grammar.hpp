#pragma once

#include <cstddef>
#include <functional>
#include <ranges>

#include "config.hpp"
#include "rule.hpp"

namespace lrk_parser {
namespace details {
struct Grammar {
  /*======== Constants =========*/
  static constexpr const CharT kStarSym = CharT{'@'};
  static constexpr const StringViewT kArrow = StringViewT{"->"};
  static constexpr const std::size_t kArrowPos = 1;
  static constexpr const std::size_t kMinSize = 1 + kArrow.size();

  /*========= Factory ==========*/
  [[nodiscard]] static Grammar init_with_strs(StringT terminals,
                                              StringT nonterminals,
                                              VectorT<StringT> rules_str,
                                              CharT start);

  /*== Symbol classification ===*/
  [[nodiscard]] bool is_terminal(CharT sym) const noexcept;

  [[nodiscard]] bool is_nonterminal(CharT sym) const noexcept;

  /*======= Rule lookup ========*/
  [[nodiscard]] bool is_rules_exists(CharT sym) const noexcept;

  [[nodiscard]] const VectorT<std::size_t>& get_rules_idxs(
      CharT sym) const noexcept;

  [[nodiscard]] const details::Rule& get_rule_by_idx(
      std::size_t rule_idx) const noexcept;

  [[nodiscard]] auto get_all_symbols_range() const {
    return all_sets | std::ranges::views::transform([](auto s) -> const auto& {
             return s.get();
           }) |
           std::ranges::views::join;
  }

  /*========== Impls ===========*/
 private:
  static void prepare_rules_str(VectorT<StringT>& rules_str);

  [[nodiscard]] static details::Rule rule_from_str(const StringT& rule_str);

  void add_rule(CharT lhs, StringT rhs);

  void add_rule(details::Rule rule);

  void throw_if_wrong_terminal_nontermianls() const;

  void throw_if_wrong_rule_str(const StringT& rule_str) const;

  /*======= Data fields ========*/
 public:
  UsetT<CharT> terminals;
  UsetT<CharT> nonterminals;
  VectorT<details::Rule> rules;
  UmapT<CharT, VectorT<std::size_t>> lhs_to_rule_idxs;

  VectorT<std::reference_wrapper<const UsetT<CharT>>> all_sets{
      std::cref(terminals), std::cref(nonterminals)};
};
}  // namespace details

using Grammar = details::Grammar;

}  // namespace lrk_parser
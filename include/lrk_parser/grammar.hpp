#pragma once

#include <cstddef>
#include <format>
#include <ostream>

#include "config.hpp"
#include "rule.hpp"

namespace lrk_parser {
namespace details {
struct Grammar {
  /*======================== Constants =========================*/
  static constexpr const CharT kStarSym = CharT{'@'};
  static constexpr const StringViewT kArrow = StringViewT{"->"};
  static constexpr const std::size_t kArrowPos = 1;
  static constexpr const std::size_t kMinSize = 1 + kArrow.size();

  /*================= Constructors/Destructors =================*/
  Grammar() = default;

  Grammar(const Grammar& /*unused*/) = default;

  Grammar(Grammar&& /*unused*/) = default;

  ~Grammar() = default;

  /*======================= Assignments ========================*/
  Grammar& operator=(const Grammar& /*unused*/) = default;

  Grammar& operator=(Grammar&& /*unused*/) = default;

  /*========================= Factory ==========================*/
  [[nodiscard]] static Grammar create_from_text(StringT terminals,
                                                StringT nonterminals,
                                                VectorT<StringT> rules_str,
                                                CharT start);

  /*========================== Output ==========================*/
  friend struct std::formatter<Grammar>;

  friend std::ostream& operator<<(std::ostream& os, const Grammar& grammar);

  /*================== Symbol classification ===================*/
  [[nodiscard]] bool is_terminal(CharT sym) const noexcept;

  [[nodiscard]] bool is_nonterminal(CharT sym) const noexcept;

  [[nodiscard]] bool is_valid_symbol(CharT sym) const noexcept;

  /*======================= Rule lookup ========================*/
  [[nodiscard]] bool is_rules_exists(CharT sym) const noexcept;

  [[nodiscard]] const VectorT<std::size_t>& get_rules_idxs(CharT sym) const;

  [[nodiscard]] const details::Rule& get_rule_by_idx(
      std::size_t rule_idx) const noexcept;

  [[nodiscard]] const VectorT<details::Rule>& get_rules() const noexcept;

  [[nodiscard]] const UsetT<CharT>& get_terminals() const noexcept;

  [[nodiscard]] const UsetT<CharT>& get_nonterminals() const noexcept;

  /*========================== Impls ===========================*/
 private:
  static void prepare_rules_str(VectorT<StringT>& rules_str);

  [[nodiscard]] static details::Rule rule_from_str(const StringT& rule_str);

  void add_rule(CharT lhs, StringT rhs);

  void add_rule(details::Rule rule);

  void throw_if_wrong_terminal_nontermianls() const;

  void throw_if_wrong_rule_str(const StringT& rule_str) const;

  /*======================= Data fields ========================*/
  UsetT<CharT> terminals_;
  UsetT<CharT> nonterminals_;
  VectorT<details::Rule> rules_;
  UmapT<CharT, VectorT<std::size_t>> lhs_to_rule_idxs_;
};
}  // namespace details

using Grammar = details::Grammar;

}  // namespace lrk_parser
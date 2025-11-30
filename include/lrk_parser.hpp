#pragma once

#include <algorithm>
#include <cstddef>
#include <deque>
#include <format>
#include <functional>
#include <ranges>
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

  template <typename T>
  using DequeT = std::deque<T>;

  struct Situation {
    [[nodiscard]] bool operator==(const Situation& /*unused*/) const noexcept =
        default;

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

    auto get_all_symbols_range() {
      return all_sets | std::views::transform([](auto s) -> const auto& {
               return s.get();
             }) |
             std::views::join;
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

    VectorT<std::reference_wrapper<const UsetT<CharT>>> all_sets{
        std::cref(terminals), std::cref(nonterminals)};
  };

  /*================= Consturctors/Destructors =================*/
  LrkParser() noexcept = default;

  LrkParser(const LrkParser& /*unused*/) = default;

  LrkParser(LrkParser&& /*unused*/) noexcept = default;

  ~LrkParser() noexcept = default;

  /*======================= Assignments ========================*/
  LrkParser& operator=(const LrkParser& /*unused*/) = default;

  LrkParser& operator=(LrkParser&& /*unused*/) noexcept = default;

  /*===================== Parser Interface =====================*/
  void fit(Grammar grammar) {
    grammar_ = std::move(grammar);

    build_all_sets_situations();
  }

  [[nodiscard]] bool predict(const StringT& word);

 private:
  /*========================== Impls ===========================*/
  void build_all_sets_situations() {
    const auto kEmptyIdx = add_string("");

    UsetT<std::size_t> visited_actpref_idx;
    DequeT<std::size_t> actpref_idx_to_visited;

    actpref_idx_to_visited.emplace_back(kEmptyIdx);

    while (not actpref_idx_to_visited.empty()) {
      auto curr_actpref_idx = actpref_idx_to_visited.front();
      actpref_idx_to_visited.pop_front();

      if (visited_actpref_idx.contains(curr_actpref_idx)) {
        continue;
      }
      visited_actpref_idx.emplace(curr_actpref_idx);
      build_one_set_situations(curr_actpref_idx);

      for (const auto& sym : grammar_.get_all_symbols_range()) {
        auto actpref_idx = add_string(idx_to_string_[curr_actpref_idx] + sym);
        actpref_idx_to_visited.emplace_back(actpref_idx);
      }
    }
  }

  void build_one_set_situations(std::size_t string_idx) {}

  std::size_t add_string(StringT string) {
    auto [it, emplace_status] =
        string_to_idx_.try_emplace(string, idx_to_string_.size());
    if (emplace_status) {
      idx_to_string_.emplace_back(std::move(string));
    }
    return it->second;
  }

  /*======================= Data fields ========================*/
  Grammar grammar_;
  UmapT<std::size_t, Situations> actpref_to_situations;
  VectorT<StringT> idx_to_string_;
  UmapT<StringT, std::size_t> string_to_idx_;
};
}  // namespace lrk_parser
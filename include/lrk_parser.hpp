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
    StringT actpref{};
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
    static constexpr const CharT kStarSym = CharT{'@'};
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

    auto get_all_symbols_range() {
      return all_sets | std::views::transform([](auto s) -> const auto& {
               return s.get();
             }) |
             std::views::join;
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
   public:
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
  void fit(Grammar grammar, std::size_t k) {
    grammar_ = std::move(grammar);
    k_ = k;

    initialize_first_k_sets();
    compute_first_k_fixed_point();
    create_initial_situations();
  }

  [[nodiscard]] bool predict(const StringT& word);

 private:
  /*========================== Impls ===========================*/
  void initialize_first_k_sets() {
    for (const auto& terminal : grammar_.terminals) {
      first_k_[terminal] = {{terminal}};
    }
  }

  void compute_first_k_fixed_point() {
    while (update_first_k_in_single_iteration());
  }

  bool update_first_k_in_single_iteration() {
    bool changed = false;
    for (const auto& rule : grammar_.rules) {
      auto rhs_first_k = compute_first_k_for_rhs(rule);
      changed |= update_lhs_first_k_if_changed(rule, rhs_first_k);
    }
    return changed;
  }

  [[nodiscard]] UsetT<StringT> compute_first_k_for_rhs(const Rule& rule) {
    UsetT<StringT> rhs_first_k_result = {StringT{}};

    for (const auto& sym : rule.rhs) {
      auto sym_first_k = first_k_[sym];
      rhs_first_k_result = concat_k_sets(rhs_first_k_result, sym_first_k);
    }

    return rhs_first_k_result;
  }

  bool update_lhs_first_k_if_changed(const Rule& rule,
                                     const UsetT<StringT>& rhs_first_k) {
    auto& lhs_first_k = first_k_[rule.lhs];
    const auto kSizeBefore = lhs_first_k.size();
    union_k_sets(lhs_first_k, rhs_first_k);
    const auto kSizeAfter = lhs_first_k.size();
    return kSizeBefore != kSizeAfter;
  }

  [[nodiscard]] UsetT<StringT> concat_k_sets(const UsetT<StringT>& lhs_set,
                                             const UsetT<StringT>& rhs_set) {
    UsetT<StringT> result;
    for (const auto& lhs : lhs_set) {
      if (lhs.size() >= k_) {
        result.emplace(lhs.substr(0, k_));
        continue;
      }
      for (const auto& rhs : rhs_set) {
        StringT added = lhs + rhs;
        if (added.size() > k_) {
          added.resize(k_);
        }
        result.emplace(std::move(added));
      }
    }
    return result;
  }

  void union_k_sets(UsetT<StringT>& lhs_set, const UsetT<StringT>& rhs_set) {
    for (const auto& rhs : rhs_set) {
      lhs_set.emplace(rhs);
    }
  }

  UsetT<StringT> compute_first_k_of_str(const StringT& str) {
    UsetT<StringT> result = {StringT{}};
    for (const auto& sym : str) {
      result = concat_k_sets(result, first_k_[sym]);
    }
    return result;
  }

  void create_initial_situations() {
    const auto& init_rule_idxs = grammar_.get_rules_idxs(Grammar::kStarSym);
    auto& init_situations = actpref_to_situations[""];
    for (const auto& rule_idx : init_rule_idxs) {
      const auto& rule = grammar_.rules[rule_idx];
      init_situations.emplace(
          Situation{.rule_idx = rule_idx, .dot_pose = 0, .actpref = ""});
    }
    closure_initial_situations();
  }

  void closure_initial_situations() {
    auto& init_situations = actpref_to_situations[""];

    DequeT<Situation> queue;
    for (const auto& situation : init_situations) {
      queue.emplace_front(situation);
    }

    while (not queue.empty()) {
      auto [rule_idx, dot_pose, actpref] = queue.back();
      queue.pop_back();
      const auto& rhs = grammar_.rules[rule_idx].rhs;
      if (dot_pose >= rhs.size()) {
        continue;
      }
      const auto& sym = rhs[dot_pose];
      if (not grammar_.is_nonterminal(sym)) {
        continue;
      }
      auto alpha = rhs.substr(dot_pose + 1);
      include_all_situations(queue, sym, alpha + actpref);
    }
  }

  void include_all_situations(DequeT<Situation>& queue, CharT lhs,
                              const StringT& actpref) {
    auto& init_situations = actpref_to_situations[""];

    auto firsk_k = compute_first_k_of_str(actpref);
    for (const auto& rule_idx : grammar_.get_rules_idxs(lhs)) {
      for (const auto& x : firsk_k) {
        Situation new_sit = {.rule_idx = rule_idx, .dot_pose = 0, .actpref = x};
        auto [it, emplace_status] = init_situations.emplace(new_sit);
        if (emplace_status) {
          queue.emplace_front(new_sit);
        }
      }
    }
  }

  void compute_goto_situations() {}

  /*======================= Data fields ========================*/
  Grammar grammar_;
  UmapT<StringT, Situations> actpref_to_situations;
  VectorT<StringT> actpref_pool_;

  std::size_t k_{};
  UmapT<CharT, UsetT<StringT>> first_k_{};
};
}  // namespace lrk_parser
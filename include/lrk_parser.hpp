#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace lrk_parser {
class LrkParser {
  /*====================== Usings/Helpers ======================*/
  using CharT = char;
  using StringT = std::basic_string<CharT>;

  template <typename T>
  using VectorT = std::vector<T>;

  template <typename T, typename H = std::hash<T>>
  using UsetT = std::unordered_set<T, H>;

  template <typename T, typename U, typename H = std::hash<T>>
  using UmapT = std::unordered_map<T, U, H>;

  struct Situation {
    std::size_t rule_idx{};
    std::size_t dot_pose{};
    std::size_t lookahead_id{};
  };

  struct SituationHash {
    /*=========== Hash ===========*/
    [[nodiscard]] std::size_t operator()(const Situation& sit) const noexcept;
  };

  using Situations = UsetT<Situation, SituationHash>;

  using SymID = std::size_t;
  using SymIDs = VectorT<SymID>;

  struct Rule {
    SymID lhs{};
    SymIDs rhs;
  };

  using Rules = VectorT<Rule>;

  struct Grammar {
    /*======== Constants =========*/
    static constexpr SymID kStartID = 0;

    /*========= Factory ==========*/
    static Grammar init_with_str(StringT terminals, StringT nonterminals,
                                 VectorT<StringT> rules_str, CharT start) {}

    /*======= Data fields ========*/
    UmapT<CharT, SymID> sym_to_id_;
    UmapT<SymID, CharT> id_to_sym_;
    UsetT<SymID> terminals_;
    UsetT<SymID> nonterminals_;
    Rules rules_;
    UmapT<SymID, VectorT<std::size_t>> lhs_id_to_rule_idxs;
    VectorT<SymIDs> lookahead_pool_;
  };
};
}  // namespace lrk_parser
#pragma once

#include <cstddef>
#include <span>
#include <unordered_map>
#include <unordered_set>

#include "config.hpp"
#include "details/prepared_grammar.hpp"

namespace lrk_parser::details {

using PrefixSet = std::unordered_set<StringT>;

class FirstK {
 public:
  [[nodiscard]] static FirstK Compute(const PreparedGrammar& grammar,
                                      std::size_t k);

  std::size_t Lookahead() const { return lookahead_; }
  const std::unordered_map<SymbolId, PrefixSet>& Sets() const { return sets_; }

  PrefixSet ForSequence(std::span<const SymbolId> symbols,
                        StringViewT lookahead = {}) const;

 private:
  explicit FirstK(std::size_t k) : lookahead_(k) {}

  std::size_t lookahead_;
  std::unordered_map<SymbolId, PrefixSet> sets_;
};

}  // namespace lrk_parser::details

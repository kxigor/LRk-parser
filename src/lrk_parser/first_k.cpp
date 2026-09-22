#include "lrk_parser/first_k.hpp"

namespace lrk_parser::details {
namespace {

PrefixSet ConcatK(const PrefixSet& lhs, const PrefixSet& rhs, std::size_t k) {
  PrefixSet result;
  for (const auto& prefix : lhs) {
    if (prefix.size() >= k) {
      result.insert(prefix.substr(0, k));
      continue;
    }
    const auto remaining = k - prefix.size();
    for (const auto& suffix : rhs) {
      result.insert(prefix + suffix.substr(0, remaining));
    }
  }
  return result;
}

}  // namespace

PrefixSet FirstK::ForSequence(std::span<const SymbolId> symbols,
                              StringViewT lookahead) const {
  PrefixSet result{StringT{}};
  for (auto symbol : symbols) {
    result = ConcatK(result, sets_.at(symbol), lookahead_);
  }
  for (CharT symbol : lookahead) {
    result = ConcatK(result, sets_.at(EncodeSymbol(symbol)), lookahead_);
  }
  return result;
}

FirstK FirstK::Compute(const PreparedGrammar& grammar, std::size_t k) {
  FirstK result(k);
  for (auto terminal : grammar.Terminals()) {
    result.sets_[terminal] = {StringT{DecodeSymbol(terminal)}};
  }
  for (const auto& rule : grammar.Rules()) {
    result.sets_.try_emplace(rule.lhs);
  }

  bool changed;
  do {
    changed = false;
    for (const auto& rule : grammar.Rules()) {
      const auto rhs = result.ForSequence(rule.rhs);
      auto& lhs = result.sets_.at(rule.lhs);
      const auto previous_size = lhs.size();
      lhs.insert(rhs.begin(), rhs.end());
      changed |= lhs.size() != previous_size;
    }
  } while (changed);
  return result;
}

}  // namespace lrk_parser::details

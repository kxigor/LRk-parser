#pragma once

#include <algorithm>
#include <cstddef>
#include <ostream>
#include <type_traits>
#include <vector>

#include "lrk_parser/config.hpp"
#include "lrk_parser/details/ids.hpp"

namespace lrk_parser::output {

inline void PrintSymbol(std::ostream& os, CharT symbol) {
  const auto value = static_cast<std::make_unsigned_t<CharT>>(symbol);
  if (symbol == '\\' || symbol == '"') {
    os << '\\' << symbol;
  } else if (symbol == '\n') {
    os << "\\n";
  } else if (symbol == '\r') {
    os << "\\r";
  } else if (symbol == '\t') {
    os << "\\t";
  } else if (value >= 0x20 && value <= 0x7e) {
    os << symbol;
  } else {
    constexpr char kHex[] = "0123456789ABCDEF";
    os << "\\x";
    for (std::size_t digit = sizeof(CharT) * 2; digit > 0; --digit) {
      os << kHex[(value >> ((digit - 1) * 4)) & 0xf];
    }
  }
}

inline void PrintSymbol(std::ostream& os, details::SymbolId symbol) {
  if (symbol == details::kAugmentedStart) {
    os << "<start>";
  } else {
    PrintSymbol(os, details::DecodeSymbol(symbol));
  }
}

inline void PrintWord(std::ostream& os, StringViewT word) {
  if (word.empty()) {
    os << "ε";
    return;
  }
  for (CharT symbol : word) {
    PrintSymbol(os, symbol);
  }
}

inline void PrintQuoted(std::ostream& os, StringViewT word) {
  if (word.empty()) {
    os << "ε";
    return;
  }
  os << '"';
  PrintWord(os, word);
  os << '"';
}

inline void PrintAlphabet(std::ostream& os, StringViewT alphabet) {
  std::vector<CharT> symbols{alphabet.begin(), alphabet.end()};
  std::ranges::sort(symbols, [](CharT lhs, CharT rhs) {
    return static_cast<std::make_unsigned_t<CharT>>(lhs) <
           static_cast<std::make_unsigned_t<CharT>>(rhs);
  });
  os << '{';
  bool first = true;
  for (CharT symbol : symbols) {
    if (!first) {
      os << ", ";
    }
    PrintSymbol(os, symbol);
    first = false;
  }
  os << '}';
}

inline bool WordLess(StringViewT lhs, StringViewT rhs) {
  return std::lexicographical_compare(
      lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
      [](CharT left, CharT right) {
        return static_cast<std::make_unsigned_t<CharT>>(left) <
               static_cast<std::make_unsigned_t<CharT>>(right);
      });
}

}  // namespace lrk_parser::output

#include <algorithm>
#include <ostream>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "format.hpp"
#include "lrk_parser/output.hpp"

namespace lrk_parser::details {

std::ostream& operator<<(std::ostream& os, const TransitionMap& transitions) {
  std::vector<std::pair<TransitionKey, StateId>> entries{transitions.begin(),
                                                         transitions.end()};
  std::ranges::sort(entries, [](const auto& lhs, const auto& rhs) {
    return std::tie(lhs.first.current_state_id, lhs.first.symbol, lhs.second) <
           std::tie(rhs.first.current_state_id, rhs.first.symbol, rhs.second);
  });
  for (const auto& [key, target] : entries) {
    os << "  " << key << " -> " << std::to_underlying(target) << '\n';
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Action& action) {
  if (const auto* shift = std::get_if<Shift>(&action.value)) {
    return os << 'S' << std::to_underlying(shift->next_state);
  }
  if (const auto* reduce = std::get_if<Reduce>(&action.value)) {
    return os << 'R' << std::to_underlying(reduce->rule);
  }
  return os << "ACC";
}

std::ostream& operator<<(std::ostream& os, const ActionKey& key) {
  os << '(' << std::to_underlying(key.state_id) << ", ";
  output::PrintQuoted(os, key.lookahead);
  return os << ')';
}

std::ostream& operator<<(std::ostream& os, const ActionTable& table) {
  std::vector<std::pair<ActionKey, Action>> entries{table.action_table_.begin(),
                                                    table.action_table_.end()};
  std::ranges::sort(entries, [](const auto& lhs, const auto& rhs) {
    if (lhs.first.state_id != rhs.first.state_id) {
      return lhs.first.state_id < rhs.first.state_id;
    }
    return output::WordLess(lhs.first.lookahead, rhs.first.lookahead);
  });
  os << "Action table:\n";
  for (const auto& [key, action] : entries) {
    os << "  " << key << " = " << action << '\n';
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const GotoTable& table) {
  return os << "Goto table:\n" << table.goto_table_;
}

}  // namespace lrk_parser::details

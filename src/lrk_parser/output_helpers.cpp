#include "lrk_parser/output_helpers.hpp"

std::ostream& lrk_parser::details::operator<<(
    std::ostream& os, const lrk_parser::details::Rule& rule) {
  os << rule.lhs << lrk_parser::Grammar::kArrow;
  if (rule.rhs.empty()) {
    os << "ε";
  } else {
    os << rule.rhs;
  }
  return os;
}
std::ostream& lrk_parser::details::operator<<(std::ostream& os,
                                              const UsetT<StringT>& set) {
  os << "{";
  bool first = true;
  for (const auto& item : set) {
    if (!first) os << ", ";
    if (item.empty()) {
      os << "ε";
    } else {
      os << "\"" << item << "\"";
    }
    first = false;
  }
  os << "}";
  return os;
}

std::ostream& lrk_parser::details::operator<<(std::ostream& os,
                                              const FirstK& first_k_obj) {
  os << "--- First-K Sets (K = " << first_k_obj.k_ << ") ---\n";

  for (const auto& pair : first_k_obj.first_k_) {
    const auto& symbol = pair.first;
    const auto& first_k_set = pair.second;
    os << "  First_" << first_k_obj.k_ << "(";
    os << symbol << ") = ";
    os << first_k_set;
    os << "\n";
  }

  os << "---------------------------\n";
  return os;
}

std::ostream& lrk_parser::details::operator<<(
    std::ostream& os, const lrk_parser::details::Grammar& grammar) {
  os << "--- LR(k) Grammar ---\n";

  os << "Non-Terminals (N) = {";
  bool first = true;
  for (const auto& sym : grammar.nonterminals) {
    if (!first) os << ", ";
    os << sym;
    first = false;
  }
  os << "}\n";

  os << "Terminals (T)     = {";
  first = true;
  for (const auto& sym : grammar.terminals) {
    if (!first) os << ", ";
    os << sym;
    first = false;
  }
  os << "}\n";

  os << "Production Rules (P):\n";

  for (const auto& [lhs, rule_idxs] : grammar.lhs_to_rule_idxs) {
    os << "  " << lhs << " -> ";

    bool first_rhs = true;
    for (std::size_t idx : rule_idxs) {
      if (!first_rhs) os << " | ";
      const auto& rule = grammar.get_rule_by_idx(idx);
      if (rule.rhs.empty()) {
        os << "ε";
      } else {
        os << rule.rhs;
      }
      first_rhs = false;
    }
    os << "\n";
  }

  if (!grammar.rules.empty() && grammar.rules[0].lhs == Grammar::kStarSym) {
    os << "Start Symbol (S)  = " << grammar.rules[0].rhs;
    os << " (Augmented: " << grammar.rules[0] << ")\n";
  }

  os << "-----------------------\n";
  return os;
}

std::ostream& lrk_parser::details::operator<<(std::ostream& os,
                                              const Situation& sit) {
  os << "[";
  os << "Rule=" << sit.rule_idx;
  os << ", Dot=" << sit.dot_pose;
  os << ", Lookahead=\"";

  if (sit.actpref.empty()) {
    os << "ε";
  } else {
    os << sit.actpref;
  }

  os << "\"]";
  return os;
}

std::ostream& lrk_parser::details::operator<<(std::ostream& os,
                                              const Situations& sits) {
  os << "{\n";
  bool first = true;

  for (const auto& sit : sits) {
    if (!first) {
      os << ",\n";
    }
    os << "    " << sit;
    first = false;
  }

  if (!sits.empty()) {
    os << "\n";
  }

  os << "}";
  return os;
}

std::ostream& lrk_parser::details::operator<<(std::ostream& os,
                                              const TransitionKey& tkey) {
  os << "(" << tkey.current_state_id << ", " << tkey.symbol << ")";
  return os;
}

std::ostream& lrk_parser::details::operator<<(std::ostream& os,
                                              const CanonicalCollection& cc) {
  os << "--- Canonical LR(K) Collection ---\n";

  os << "## States:\n";
  const auto& states = cc.get_states();
  for (StateIdT i = 0; i < states.size(); ++i) {
    os << "  State " << i << ":\n";
    os << states[i] << "\n";
  }

  os << "\n## Goto Table:\n";
  const auto& goto_table = cc.get_goto_table();

  if (goto_table.empty()) {
    os << "  (Empty)\n";
  } else {
    UmapT<StateIdT, VectorT<std::pair<CharT, StateIdT>>> sorted_transitions;
    for (const auto& pair : goto_table) {
      const auto& tkey = pair.first;
      const auto& next_state_id = pair.second;
      sorted_transitions[tkey.current_state_id].emplace_back(tkey.symbol,
                                                             next_state_id);
    }

    for (const auto& state_pair : sorted_transitions) {
      const StateIdT current_state_id = state_pair.first;
      const auto& transitions = state_pair.second;

      for (const auto& transition : transitions) {
        os << "  GOTO(" << current_state_id << ", " << transition.first
           << ") = " << transition.second << "\n";
      }
    }
  }

  os << "--------------------------------\n";
  return os;
}
#include "lrk_parser/output_helpers.hpp"

#include <iomanip>

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

std::ostream& lrk_parser::details::operator<<(std::ostream& os,
                                              const ActionType& type) {
  switch (type) {
    case ActionType::Shift:
      return os << "Shift";
    case ActionType::Reduce:
      return os << "Reduce";
    case ActionType::Accept:
      return os << "Accept";
    // Добавьте другие типы, если они существуют, например Error
    case ActionType::Error:
    default:
      return os << "Error";
  }
}

// 2. Вывод действия (Action)
// Action: {.type, .value}
std::ostream& lrk_parser::details::operator<<(std::ostream& os,
                                              const Action& action) {
  switch (action.type) {
    case ActionType::Shift:
      // Shift to state X
      os << "S" << action.value;
      break;
    case ActionType::Reduce:
      // Reduce by rule X
      os << "R" << action.value;
      break;
    case ActionType::Accept:
      // Accept
      os << "ACC";
      break;
    case ActionType::Error:
    default:
      // В случае ошибки или неопределенного типа
      os << "ERR";
      break;
  }
  return os;
}

// 3. Вывод ключа действия (ActionKey)
// Выводит в формате: (State_ID, "Lookahead")
std::ostream& lrk_parser::details::operator<<(std::ostream& os,
                                              const ActionKey& a_key) {
  os << "(" << a_key.state_id << ", \"";
  if (a_key.lookahead.empty()) {
    os << "ε";
  } else {
    os << a_key.lookahead;
  }
  os << "\")";
  return os;
}

// 4. Основной оператор вывода для ActionTable
std::ostream& lrk_parser::details::operator<<(std::ostream& os,
                                              const ActionTable& table) {
  os << "--- LR(K) Action Table ---\n";

  const auto& action_map = table.action_table_;

  if (action_map.empty()) {
    os << "  (Table is empty)\n";
    os << "--------------------------\n";
    return os;
  }

  // Сбор уникальных состояний и уникальных lookahead'ов
  // Для построения таблицы в удобном матричном формате
  UsetT<StateIdT> unique_states;
  UsetT<StringT> unique_lookaheads;

  for (const auto& pair : action_map) {
    unique_states.insert(pair.first.state_id);
    unique_lookaheads.insert(pair.first.lookahead);
  }

  // --- Форматированный вывод таблицы ---

  // Определяем ширину столбца
  const int kColWidth = 10;

  // Строка заголовков (Lookaheads)
  os << std::setw(kColWidth) << std::left << "State";
  for (const auto& lookahead : unique_lookaheads) {
    std::string header = lookahead;
    if (lookahead.empty()) {
      header = "ε";  // Или "$" для EOF, если используется как lookahead
    }
    os << std::setw(kColWidth) << std::left << header;
  }
  os << "\n";

  // Горизонтальная линия
  os << std::string(kColWidth * (unique_lookaheads.size() + 1), '-') << "\n";

  // Строки состояний
  for (const auto& state_id : unique_states) {
    os << std::setw(kColWidth) << std::left << state_id;

    for (const auto& lookahead : unique_lookaheads) {
      ActionKey a_key = {.state_id = state_id, .lookahead = lookahead};

      std::stringstream ss;
      if (table.has_parse_action(a_key)) {
        ss << table.get_parse_action(a_key);
      } else {
        ss << "";
      }

      os << std::setw(kColWidth) << std::left << ss.str();
    }
    os << "\n";
  }

  os << "--------------------------------\n";
  return os;
}
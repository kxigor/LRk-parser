#pragma once

#include <ostream>

#include "action_table.hpp"
#include "canonical_collection.hpp"
#include "config.hpp"
#include "first_k.hpp"
#include "goto_table.hpp"
#include "grammar.hpp"
#include "parser.hpp"
#include "rule.hpp"
#include "situation.hpp"
#include "tables_base.hpp"

namespace lrk_parser {
/*=========================== Fwds ===========================*/
std::ostream& operator<<(std::ostream& os, const Rule& rule);
std::ostream& operator<<(std::ostream& os, const Grammar& grammar);
namespace details {
std::ostream& operator<<(std::ostream& os, const Situation& sit);
std::ostream& operator<<(std::ostream& os, const Situations& sits);
std::ostream& operator<<(std::ostream& os, const UsetT<StringT>& set);
std::ostream& operator<<(std::ostream& os, const FirstK& first_k_obj);
std::ostream& operator<<(std::ostream& os, const TransitionKey& tkey);
std::ostream& operator<<(std::ostream& os, const CanonicalCollection& cc);
std::ostream& operator<<(std::ostream& os, const ActionType& type);
std::ostream& operator<<(std::ostream& os, const Action& action);
std::ostream& operator<<(std::ostream& os, const ActionKey& a_key);
std::ostream& operator<<(std::ostream& os, const GotoTable& table);
std::ostream& operator<<(std::ostream& os, const ActionTable& table);
}  // namespace details
std::ostream& operator<<(std::ostream& os, const LrkParser& parser);
}  // namespace lrk_parser
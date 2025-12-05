#pragma once

#include "canonical_collection.hpp"
#include "config.hpp"
#include "first_k.hpp"
#include "grammar.hpp"
#include "rule.hpp"
#include "situation.hpp"
#include "tables_base.hpp"
#include "action_table.hpp"

namespace lrk_parser::details {
/*=========================== Fwds ===========================*/
std::ostream& operator<<(std::ostream& os, const Rule& rule);
std::ostream& operator<<(std::ostream& os, const Situation& sit);
std::ostream& operator<<(std::ostream& os, const Situations& sits);
std::ostream& operator<<(std::ostream& os, const UsetT<StringT>& set);
std::ostream& operator<<(std::ostream& os, const Grammar& grammar);
std::ostream& operator<<(std::ostream& os, const FirstK& first_k_obj);
std::ostream& operator<<(std::ostream& os, const TransitionKey& tkey);
std::ostream& operator<<(std::ostream& os, const CanonicalCollection& cc);
std::ostream& operator<<(std::ostream& os, const ActionType& type);
std::ostream& operator<<(std::ostream& os, const Action& action);
std::ostream& operator<<(std::ostream& os, const ActionKey& a_key);
std::ostream& operator<<(std::ostream& os, const ActionTable& table);

}  // namespace lrk_parser::details
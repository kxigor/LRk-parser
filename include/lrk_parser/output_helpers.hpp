#pragma once

#include "canonical_collection.hpp"
#include "config.hpp"
#include "first_k.hpp"
#include "grammar.hpp"
#include "rule.hpp"
#include "situation.hpp"
#include "tables_base.hpp"

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

}  // namespace lrk_parser::details
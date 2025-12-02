#pragma once

#include "config.hpp"
#include "situation.hpp"
#include "tables_base.hpp"

namespace lrk_parser::details {
class States {
  VectorT<details::Situations> states_;

  UmapT<details::Situations, StateIdT, details::SituationsHash>
      state_set_to_id_;
};
}  // namespace lrk_parser::details
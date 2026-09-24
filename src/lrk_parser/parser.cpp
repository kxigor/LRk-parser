#include "lrk_parser/parser.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

#include "lrk_parser/canonical_collection.hpp"
#include "lrk_parser/config.hpp"
#include "lrk_parser/first_k.hpp"
#include "lrk_parser/grammar.hpp"
#include "lrk_parser/tables_base.hpp"

using LrkParser = lrk_parser::LrkParser;
using ActionType = lrk_parser::details::ActionType;

void LrkParser::fit(const Grammar& grammar, std::size_t k) {
  grammar_.emplace(grammar);
  /*
  Мы  это  делаем, потому что я нигде не храню лишнюю информацию по символам,
  а   для   k = 0   таблицы   выраждаются   и   мне,  чтобы  проверять  вход,
  нужно  явно  хардкодить  везде  Lookahead  чтобы  был  следующим  символом,
  либо  создавать  проверки  стэка.  Если  этого  не сделать, получится, что
  распознаётся  любой  однобуквкнный  символ. Этот хардкод, который я описал
  эквивалентен тому, чтобы поставить k = 1, поэтому такое шение было принято
  */
  const std::size_t kKfinal = std::max(1UL, k);
  fit_impl(kKfinal);
}

void lrk_parser::LrkParser::fit_impl(std::size_t k) {
  k_ = k;
  const auto kFirstK = details::FirstK::Compute(*grammar_, k_);
  auto lr_collection = details::CanonicalCollection::Build(*grammar_, kFirstK);

  action_table_ = details::ActionTable(*grammar_, kFirstK, lr_collection);
  goto_table_ = details::GotoTable(std::move(lr_collection));
}

bool LrkParser::predict(const StringT& word) const {
  PredictContext ctx(word);

  while (ctx.is_processing_word) {
    auto action_res = get_next_action(ctx);
    if (not action_res.has_value()) {
      break;
    }

    // clang-format off
    switch (action_res->type) {
      case ActionType::Shift  : { handle_shift_case (ctx, action_res->value); } break;
      case ActionType::Reduce : { handle_reduce_case(ctx, action_res->value); } break;
      case ActionType::Accept : { handle_accept_case(ctx);                    } break;
      case ActionType::Error  : { handle_error_case (ctx);                    } break;
      default                 : { std::unreachable();                         } break;
    }
    // clang-format on
  }

  return ctx.is_word_recognized;
}

lrk_parser::OptionalT<lrk_parser::details::Action> LrkParser::get_next_action(
    PredictContext& ctx) const {
  const auto kCurrentStateId = ctx.stack.back();

  StringT u;
  if (ctx.cursor < ctx.word.size()) {
    u = ctx.word.substr(ctx.cursor, k_);
  }

  const auto kAKey =
      details::ActionKey{.state_id = kCurrentStateId, .lookahead = u};

  if (not action_table_.has_parse_action(kAKey)) {
    return {};
  }

  return action_table_.get_parse_action(kAKey);
}

void LrkParser::handle_shift_case(PredictContext& ctx,
                                  std::size_t next_state_id) {
  if (ctx.cursor >= ctx.word.size()) {
    ctx.is_processing_word = false;
    ctx.is_word_recognized = false;
    return;
  }

  ctx.stack.push_back(details::StateId{next_state_id});

  ++ctx.cursor;
}

void LrkParser::handle_reduce_case(PredictContext& ctx,
                                   std::size_t next_state_id) const {
  const auto& rule = grammar_->GetRule(details::RuleId{next_state_id});
  const auto kSymsToPop = rule.rhs.size();

  if (ctx.stack.size() < kSymsToPop + 1) {
    ctx.is_processing_word = false;
    ctx.is_word_recognized = false;
    return;
  }

  for (std::size_t i = 0; i < kSymsToPop; ++i) {
    ctx.stack.pop_back();
  }

  const details::StateId kStateTop = ctx.stack.back();

  const details::TransitionKey kGotoKey{.current_state_id = kStateTop,
                                        .symbol = rule.lhs};

  if (not goto_table_.has_goto_state(kGotoKey)) {
    ctx.is_processing_word = false;
    ctx.is_word_recognized = false;
    return;
  }

  const details::StateId kNextState = goto_table_.get_goto_state(kGotoKey);

  ctx.stack.push_back(kNextState);
}

void LrkParser::handle_accept_case(PredictContext& ctx) {
  ctx.is_processing_word = false;
  ctx.is_word_recognized = (ctx.cursor == ctx.word.size());
}

void LrkParser::handle_error_case(PredictContext& ctx) {
  ctx.is_processing_word = false;
  ctx.is_word_recognized = false;
}

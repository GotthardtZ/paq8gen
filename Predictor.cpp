#include "Predictor.hpp"

Predictor::Predictor(Shared* const sh) : shared(sh), models(sh), contextModel(sh, models), sse(sh), pr(0) {
  shared->reset();
}

auto Predictor::p() -> int { 
  // predict
  pr = contextModel.p();
  // SSE Stage
  pr = sse.p(pr);
  return pr;
}

void Predictor::update(uint8_t y) {
  // update global context: y, pos, bitPosition, c0, c4, c8, buf
  shared->update(y);
}

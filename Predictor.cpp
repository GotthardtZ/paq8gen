#include "Predictor.hpp"

Predictor::Predictor(Shared* const sh) : shared(sh), models(sh), contextModel(sh, models), sse(sh), pr(2048) {
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
  shared->State.misses = shared->State.misses<<1 | static_cast<uint64_t>((pr >> 11U) != y);
}

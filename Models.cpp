#include "Models.hpp"

/*
 relationship between compression level, shared->mem and NormalModel memory use as an example

 level   shared->mem    NormalModel memory use (shared->mem*32)
 -----   -----------    ----------------------
   1      0.125 MB              4 MB
   2      0.25	MB              8 MB
   3      0.5   MB             16 MB
   4      1.0   MB             32 MB
   5      2.0   MB             64 MB
   6      4.0   MB            128 MB
   7      8.0   MB            256 MB
   8     16.0   MB            512 MB
   9     32.0   MB           1024 MB
  10     64.0   MB           2048 MB
  11    128.0   MB           4096 MB
  12    256.0   MB           8192 MB

*/


Models::Models(Shared* const sh) : shared(sh) {}

auto Models::normalModel() -> NormalModel & {
  static NormalModel instance {shared, shared->mem * 64};
  return instance;
}

auto Models::matchModel() -> MatchModel & {
  static MatchModel instance {shared, shared->mem / 4 /*hashtablesize*/, shared->mem /*mapmemorysize*/ };
  return instance;
}

auto Models::lineModel() -> LineModel & {
  static LineModel instance {shared, shared->mem * 2};
  return instance;
}



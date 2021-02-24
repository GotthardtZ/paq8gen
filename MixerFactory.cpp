#include "MixerFactory.hpp"
auto MixerFactory::createMixer(const Shared* const sh, const int n, const int m, const int s) -> Mixer * {

  if( sh->chosenSimd == SIMD_NONE ) {
    return new SIMDMixer<SIMD_NONE>(sh, n, m, s);
  }
  else if( sh->chosenSimd == SIMD_SSE2 ) {
    return new SIMDMixer<SIMD_SSE2>(sh, n, m, s);
  }
  else if (sh->chosenSimd == SIMD_SSSE3) {
    return new SIMDMixer<SIMD_SSSE3>(sh, n, m, s);
  }
  else if( sh->chosenSimd == SIMD_AVX2 ) {
    return new SIMDMixer<SIMD_AVX2>(sh, n, m, s);
  }
  else if (sh->chosenSimd == SIMD_NEON) {
    return new SIMDMixer<SIMD_NEON>(sh, n, m, s);
  }
  assert(false);
  return nullptr;
}

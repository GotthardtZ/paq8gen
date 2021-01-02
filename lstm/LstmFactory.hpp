#ifndef PAQ8GEN_LSTMFACTORY_HPP
#define PAQ8GEN_LSTMFACTORY_HPP

#include "LstmModel.hpp"
#include "SimdLstmModel.hpp"
#include "../Shared.hpp"
#include "../utils.hpp"

template <std::size_t Bits = 8>
class LstmFactory {
public:
  static LstmModel<Bits>* CreateLSTM(
    const Shared* const sh,
    std::size_t const num_cells,
    std::size_t const num_layers,
    std::size_t const horizon,
    float const learning_rate,
    float const gradient_clip)
  {
    if (sh->chosenSimd == SIMD_AVX2)
      return new SIMDLstmModel<SIMD_AVX2, Bits>(sh, num_cells, num_layers, horizon, learning_rate, gradient_clip);
    else
      return new SIMDLstmModel<SIMD_NONE, Bits>(sh, num_cells, num_layers, horizon, learning_rate, gradient_clip);
  }
};

#endif //PAQ8GEN_LSTMFACTORY_HPP

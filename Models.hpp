#ifndef PAQ8GEN_MODELS_HPP
#define PAQ8GEN_MODELS_HPP

#include "model/DmcForest.hpp"
#include "model/MatchModel.hpp"
#include "model/NormalModel.hpp"
#include "model/LineModel.hpp"
#include "lstm/LstmModel.hpp"
#include "lstm/LstmFactory.hpp"

/**
 * This is a factory class for lazy object creation for models.
 * Objects created within this class are instantiated on first use and guaranteed to be destroyed.
 */
class Models {
private:
  Shared * const shared;
public:
  explicit Models(Shared* const sh);
  auto normalModel() -> NormalModel &;
  auto dmcForest() -> DmcForest &;
  auto matchModel() -> MatchModel &;
  auto lineModel() -> LineModel &;
  auto lstmModel() -> LstmModel<> &;
};

#endif //PAQ8GEN_MODELS_HPP

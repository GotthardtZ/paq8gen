#ifndef PAQ8GEN_PREDICTOR_HPP
#define PAQ8GEN_PREDICTOR_HPP

#include "file/FileDisk.hpp"
#include "Models.hpp"
#include "SSE.hpp"
#include "Shared.hpp"
#include "UpdateBroadcaster.hpp"
#include "model/ContextModel.hpp"
#include "utils.hpp"

/**
 * A Predictor estimates the probability that the next bit of uncompressed data is 1.
 */
class Predictor {
private:
    Shared *shared;
    Models models;
    ContextModel contextModel;
    SSE sse;
    int pr; // next prediction, scaled by 12 bits (0-4095)
    void trainText(const char *dictionary, int iterations);
    void trainExe();

public:
  Predictor(Shared* const sh);

    /**
     * Returns P(1) as a 12 bit number (0-4095).
     * @return the prediction
     */
    [[nodiscard]] auto p() -> int;

    /**
     * Trains the models with the actual bit (0 or 1).
     * @param y the actual bit
     */
    void update(uint8_t y);
};

#endif //PAQ8GEN_PREDICTOR_HPP

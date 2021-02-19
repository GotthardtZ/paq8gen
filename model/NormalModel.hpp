#ifndef PAQ8GEN_NORMALMODEL_HPP
#define PAQ8GEN_NORMALMODEL_HPP

#include "../ContextMap2.hpp"
#include "../IPredictor.hpp"

/**
 * Model for order 0-24 contexts
 * Contexts are hashes of previous 0..24 bytes.
 * Note: order 8+ contexts are modeled by matchModel as well.
 */
class NormalModel : IPredictor {
private:
    static constexpr int nCM = 24;
    static constexpr int nSM = 1;
    Shared * const shared;
    ContextMap2 cm;
    StateMap smOrder1;
public:
    static constexpr int MIXERINPUTS =
      2*nSM + 
      nCM * (ContextMap2::MIXERINPUTS + ContextMap2::MIXERINPUTS_RUN_STATS + ContextMap2::MIXERINPUTS_BYTE_HISTORY); //170
    static constexpr int MIXERCONTEXTS = (16 * 8) + 4 + 256 + (256 * 8) + 2048 + 4096; //8580
    static constexpr int MIXERCONTEXTSETS = 6;
    NormalModel(Shared* const sh, uint64_t cmSize);

    /**
     * update order 1..24 context hashes.
     * Note: order 0 context does not need an update so its hash never changes.
     */
    void update();
    void mix(Mixer &m);
};

#endif //PAQ8GEN_NORMALMODEL_HPP

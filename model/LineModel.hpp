#ifndef PAQ8GEN_LINEMODEL_HPP
#define PAQ8GEN_LINEMODEL_HPP

#include "../ContextMap2.hpp"
#include "../Shared.hpp"
#include <cctype>

/**
 * Model lines and columns
 */
class LineModel {
private:
  static constexpr int nCM = 4;
public:
    static constexpr int MIXERINPUTS = nCM * (ContextMap2::MIXERINPUTS + ContextMap2::MIXERINPUTS_RUN_STATS + ContextMap2::MIXERINPUTS_BYTE_HISTORY); // 28
    static constexpr int MIXERCONTEXTS = 0;
    static constexpr int MIXERCONTEXTSETS = 0;
    uint64_t nl1{};
    int firstChar{};
private:
    Shared * const shared;
    ContextMap2 cm;
public:
    LineModel(Shared* const sh, uint64_t size);
    void mix(Mixer &m);
};


#endif //PAQ8GEN_LINEMODEL_HPP

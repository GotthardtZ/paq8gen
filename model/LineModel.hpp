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
  static constexpr int nCM = 14;
public:
    static constexpr int MIXERINPUTS = nCM * (ContextMap2::MIXERINPUTS + ContextMap2::MIXERINPUTS_RUN_STATS + ContextMap2::MIXERINPUTS_BYTE_HISTORY); // 28
    static constexpr int MIXERCONTEXTS = 256 + (3 * 16 * 8);
    static constexpr int MIXERCONTEXTSETS = 2;

    static constexpr int maxLineMatch = 16;
    int lineMatch{}; /**< the length of match of the current line vs the previous line */
    uint64_t groups{}; /**< 8 last character categories */
    uint64_t nl2{};
    uint64_t nl1{};
    uint64_t line0{};
    int firstChar{};
private:
    Shared * const shared;
    ContextMap2 cm;
public:
    LineModel(Shared* const sh, uint64_t size);
    void mix(Mixer &m);
};


#endif //PAQ8GEN_LINEMODEL_HPP

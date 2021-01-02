#ifndef PAQ8GEN_CONTEXTMODEL_HPP
#define PAQ8GEN_CONTEXTMODEL_HPP

#include "../Mixer.hpp"
#include "../MixerFactory.hpp"
#include "../Models.hpp"

/**
 * This combines all the context models with a Mixer.
 */
class ContextModel {
    Shared * const shared;
    Models models;
    Mixer *m;
    BlockType nextBlockType = DEFAULT;
    BlockType blockType = DEFAULT;
    int blockSize = 1;
    int blockInfo = 0;
    int bytesRead = 0;
    bool readSize = false;

public:
    ContextModel(Shared* const sh, Models &models);
    auto p() -> int;
    ~ContextModel();
};

#endif //PAQ8GEN_CONTEXTMODEL_HPP

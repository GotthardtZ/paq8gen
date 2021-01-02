#include "ContextModel.hpp"

ContextModel::ContextModel(Shared* const sh, Models &models) : shared(sh), models(models) {
  auto mf = new MixerFactory();
  m = mf->createMixer(
    // this is the maximum case: how many mixer inputs, mixer contexts and mixer context sets are needed (max)
    sh,
    1 +  //bias
    MatchModel::MIXERINPUTS + NormalModel::MIXERINPUTS +
    LineModel::MIXERINPUTS +
    DmcForest::MIXERINPUTS + LstmModel<>::MIXERINPUTS
    ,
    MatchModel::MIXERCONTEXTS + NormalModel::MIXERCONTEXTS +
    LineModel::MIXERCONTEXTS +
    DmcForest::MIXERCONTEXTS + LstmModel<>::MIXERCONTEXTS
    ,
    MatchModel::MIXERCONTEXTSETS + NormalModel::MIXERCONTEXTSETS +
    LineModel::MIXERCONTEXTSETS +
    DmcForest::MIXERCONTEXTSETS + LstmModel<>::MIXERCONTEXTSETS
  );
}

auto ContextModel::p() -> int {
  uint32_t &blpos = shared->State.blockPos;
  // Parse block type and block size
  INJECT_SHARED_bpos
  if( bpos == 0 ) {
    --blockSize;
    blpos++;
    INJECT_SHARED_c1
    if( blockSize == -1 ) {
      nextBlockType = static_cast<BlockType>(c1); //got blockType but don't switch (we don't have all the info yet)
      bytesRead = 0;
      readSize = true;
    } else if( blockSize < 0 ) {
      if( readSize ) {
        bytesRead |= int(c1 & 0x7FU) << ((-blockSize - 2) * 7);
        if((c1 >> 7U) == 0 ) {
          readSize = false;
          if( !hasInfo(nextBlockType)) {
            blockSize = bytesRead;
            if( hasRecursion(nextBlockType)) {
              blockSize = 0;
            }
            blpos = 0;
          } else {
            blockSize = -1;
          }
        }
      } else if( blockSize == -5 ) {
        INJECT_SHARED_c4
        blockSize = bytesRead;
        blockInfo = c4;
        blpos = 0;
      }
    }

    if(blpos == 0 ) {
      blockType = nextBlockType; //got all the info - switch to next blockType
      shared->State.blockType = blockType;
    }
    if( blockSize == 0 ) {
      blockType = DEFAULT;
      shared->State.blockType = blockType;
    }
  }

  m->add(256); //network bias

  MatchModel &matchModel = models.matchModel();
  matchModel.mix(*m);
  
  NormalModel &normalModel = models.normalModel();
  normalModel.mix(*m);

  normalModel.mixPost(*m);

  if ((shared->options & OPTION_LSTM) != 0u) {
    LstmModel<>& lstmModel = models.lstmModel();
    lstmModel.mix(*m);
  }

  LineModel &lineModel = models.lineModel();
  lineModel.mix(*m);

  DmcForest &dmcForest = models.dmcForest();
  dmcForest.mix(*m);

  m->setScaleFactor(2048, 256);
  return m->p();
}

ContextModel::~ContextModel() {
  delete m;
}

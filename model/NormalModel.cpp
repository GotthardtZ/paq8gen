#include "NormalModel.hpp"

NormalModel::NormalModel(Shared* const sh, const uint64_t cmSize) : 
  shared(sh), cm(sh, cmSize, nCM, 64),
  smOrder1(sh, 1, 255 * 256, 1023, StateMap::Generic)
{
  assert(isPowerOf2(cmSize));
}

void NormalModel::updateHashes() {
  INJECT_SHARED_c1
  uint64_t* cxt = shared->State.cxt;
  for( uint64_t i = 32; i > 0; --i ) {
    cxt[i] = (cxt[i - 1] + c1 + i) * PHI64;
  }
}

void NormalModel::mix(Mixer &m) {
  INJECT_SHARED_bpos
  if( bpos == 0 ) {
    updateHashes();
    uint64_t* cxt = shared->State.cxt;
    const uint8_t RH = CM_USE_RUN_STATS | CM_USE_BYTE_HISTORY;
    for(uint64_t i = 1; i <= 24; i++ ) {
      cm.set(RH, cxt[i]);
    }
  }
  cm.mix(m);

  INJECT_SHARED_c0
  INJECT_SHARED_c1
  int p1 = smOrder1.p1((c0 - 1) << 8 | c1);
  m.add(stretch(p1)>> 2);
  m.add((p1-2048) >> 3);

  int order = shared->State.order = max(0, cm.order - (nCM - 15)); //0-15
  assert(order <= 15);
  m.set(order << 3 | bpos, 8*16);
}

void NormalModel::mixPost(Mixer &m) {
  INJECT_SHARED_c0
  INJECT_SHARED_c1
  INJECT_SHARED_c4
  INJECT_SHARED_bpos
  m.set(c0, 256);
  m.set(c1<<3 | bpos, 2048);
  m.set(shared->State.order << 4 | finalize64(hash(c1), 4), 16*16);
  m.set(finalize64(hash(c4), 8), 256);
  m.set(finalize64(hash(c4 & 0xffff, c0), 8), 256);
}

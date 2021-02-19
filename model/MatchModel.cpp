#include "MatchModel.hpp"

MatchModel::MatchModel(Shared* const sh, const uint64_t hashtablesize, const uint64_t mapmemorysize) : 
  shared(sh),
  hashtable(hashtablesize),
  stateMaps {{sh, 1, 28 * 512,          255, StateMap::Generic},
             {sh, 1, 8 * 256 * 256 + 1, 255, StateMap::Generic},
             {sh, 1, 256 * 256,         255, StateMap::Generic}},
  cm(sh, mapmemorysize, nCM, 64),
  mapL /* LargeStationaryMap : HashBits, Scale=64, Rate=16  */
        {sh, nLSM, 20} // effective bits: ~22
  ,
  hashBits(ilog2(uint32_t(hashtable.size())))
{
  assert(isPowerOf2(hashtablesize));
  assert(isPowerOf2(mapmemorysize));
}

void MatchModel::update() {

  //update active candidates, remove dead candidatesq
  size_t n = max(numberOfActiveCandidates, 1);
  for (size_t i = 0; i < n; i++) {
    MatchInfo* matchInfo = &matchCandidates[i];
    matchInfo->update(shared);
    if (numberOfActiveCandidates != 0 && matchInfo->isInNoMatchMode()) {
      numberOfActiveCandidates--;
      if (numberOfActiveCandidates == i)
        break;
      memmove(&matchCandidates[i], &matchCandidates[i + 1], (numberOfActiveCandidates - i) * sizeof(MatchInfo));
      i--;
    }
  }

  INJECT_SHARED_bpos
  if( bpos == 0 ) {
    
    INJECT_SHARED_pos
    uint64_t hash;
    HashElementForMatchPositions* matches;

    hash = shared->State.cxt[LEN4];
    matches = &hashtable[finalize64(hash, hashBits)];
    if (numberOfActiveCandidates < N)
      AddCandidates(matches, LEN4);
    matches->Add(pos);

    hash = shared->State.cxt[LEN3];
    matches = &hashtable[finalize64(hash, hashBits)];
    if (numberOfActiveCandidates < N)
      AddCandidates(matches, LEN3);
    matches->Add(pos);

    hash = shared->State.cxt[LEN2];
    matches = &hashtable[finalize64(hash, hashBits)];
    if (numberOfActiveCandidates < N)
      AddCandidates(matches, LEN2);
    matches->Add(pos);

    hash = shared->State.cxt[LEN1];
    matches = &hashtable[finalize64(hash, hashBits)];
    if (numberOfActiveCandidates < N)
      AddCandidates(matches, LEN1);
    matches->Add(pos);

    INJECT_SHARED_buf
    for (size_t i = 0; i < numberOfActiveCandidates; i++) {
      matchCandidates[i].expectedByte = buf[matchCandidates[i].index];
    }
  }
}

void MatchModel::mix(Mixer &m) {
  shared->GetUpdateBroadcaster()->subscribe(this);

  for( uint32_t i = 0; i < nST; i++ ) { // reset contexts
    ctx[i] = 0;
  }
  
  size_t bestCandidateIdx = 0; //default item is the first candidate, let's see if any other candidate is better
  for (size_t i = 1; i < numberOfActiveCandidates; i++) {
    if (matchCandidates[i].isBetterThan(&matchCandidates[bestCandidateIdx]))
      bestCandidateIdx = i;
  }

  const uint32_t length = matchCandidates[bestCandidateIdx].length;
  const uint8_t expectedByte = matchCandidates[bestCandidateIdx].expectedByte;
  const bool isInNoMatchMode= matchCandidates[bestCandidateIdx].isInNoMatchMode();
  const bool isInDeltaMode = matchCandidates[bestCandidateIdx].delta;
  const bool isInPreRecoveryMode = matchCandidates[bestCandidateIdx].isInPreRecoveryMode();
  const bool isInRecoveryMode = matchCandidates[bestCandidateIdx].isInRecoveryMode();

  INJECT_SHARED_bpos
  INJECT_SHARED_c0
  INJECT_SHARED_c1
  const int expectedBit = length != 0 ? (expectedByte >> (7 - bpos)) & 1 : 0;

  uint32_t n0 = 0;
  uint32_t n1 = 0;
  for (size_t i = 0; i < numberOfActiveCandidates; i++) {
    if (matchCandidates[i].length == 0)
      continue;
    int expectedBit = (matchCandidates[i].expectedByte >> (7 - bpos)) & 1;
    n0 += 1 - expectedBit;
    n1 += expectedBit;
  }
  
  const bool isUncertain = n0 != 0 && n1 != 0;

  uint32_t denselength = 0; // 0..27
  if (length != 0) {
    if (length <= 16) {
      denselength = length - 1; // 0..15
    } else {
      denselength = 12 + (min(length - 1u, 63u) >> 2); // 16..27
    }
    ctx[0] = (denselength << 9) | (expectedBit << 8) | c0; // 1..28*512
    ctx[1] = ((expectedByte << 11) | (bpos << 8) | c1) + 1;
    const int sign = 2 * expectedBit - 1;
    m.add(sign * ((min(length, 32u) << 5) >> isUncertain)); // +/- 32..1024
    m.add(sign * ((ilog->log(min(length, 65535u)) << 2) >> isUncertain)); // +/-  0..1024
  } else { // no match at all or delta mode
    m.add(0);
    m.add(0);
  }

  if( isInDeltaMode ) { // delta mode: helps predicting the remaining bits of a character when a mismatch occurs
    ctx[2] = (expectedByte << 8) | c0;
  }

  for( uint32_t i = 0; i < nST; i++ ) {
    const uint32_t c = ctx[i];
    if( c != 0 ) {
      const int p1 = stateMaps[i].p1(c);
      const int st = stretch(p1);
      m.add(st >> 2);
      m.add((p1 - 2048) >> 3);
    } else {
      m.add(0);
      m.add(0);
    }
  }

  const uint32_t length2 = min(length >> 3, 3u); //0-3 (2 bits)

  const uint8_t mode = 
    isInNoMatchMode ? 0 :
    isInDeltaMode ? 1 :
    isInPreRecoveryMode ? 2 :
    isInRecoveryMode ? 3 :
    4 + length2; //0-7

  const uint32_t mCtx =
    isInNoMatchMode ? 0 :
    isInDeltaMode ? 1 :
    isInPreRecoveryMode ? 2 :
    isInRecoveryMode ? 3 :
    4 + (length2 << 2 | expectedBit << 1 | isUncertain); //0..19

  //bytewise contexts
  INJECT_SHARED_c4
  if( bpos == 0 ) {
    const uint8_t R_ = CM_USE_RUN_STATS;
    cm.set(R_, hash((length != 0 ? expectedByte : c1)<<3 | mode)); //max context bits: 8+8+3 = 19
    cm.set(R_, hash(length != 0 ? expectedByte : ((c4 >> 8) & 0xff), (c1<<3)| mode)); //max context bits: 8+8+8+3=27
  }
  cm.mix(m);

  //bitwise contexts
  mapL.set(hash(c0, c4 & 0x00ffffff, mCtx)); // max context bits: 8+8+24+3 = 43 bits -> hashed into ~22 bits
  mapL.mix(m);

  
  uint32_t lengthCtx =
    length == 0 ? 0 :
    isUncertain ? 1 :
    length < 16 ? 2 :
    3;

  m.set(mCtx, 20);
  m.set(lengthCtx << 3 | bpos, 4 * 8);
  m.set((n0 * (N+1) + n1) << 3 | bpos, (N + 1) * (N + 1) * 8);

  shared->State.Match.matchCtx = mCtx; //0..19
  shared->State.Match.expectedByte = length != 0 ? expectedByte : 0;

}

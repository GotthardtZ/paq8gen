#include "LineModel.hpp"


LineModel::LineModel(Shared* const sh, const uint64_t size) : 
  shared(sh),
  cm(sh, size, nCM, 64)
{}


void LineModel::update() {
  INJECT_SHARED_bpos
  assert(bpos == 0);
  INJECT_SHARED_pos
  INJECT_SHARED_c1
  INJECT_SHARED_blockPos
  const auto isNewline = (c1 == NEW_LINE || blockPos == 0);
  if (isNewline) { // a new line has just started (or: zero in asciiz or in binary data)
    nl2 = nl1;
    nl1 = pos;
    line0 = 0;
    linePos3 = 0;
    lineType = 0;
  }
  else {
    line0 = combine64(line0, c1);
    linePos3++;
    if (linePos3 == 3)
      linePos3 = 0;
  }

  uint64_t col = pos - nl1;
  if (col == 1) {
    if (('A' <= c1 && c1 <= 'Z') || ('a' <= c1 && c1 <= 'z'))
      lineType = 1; //sequence
    else if (c1 == '>')
      lineType = 2; //sequence name
    else
      lineType = 0; //other
    shared->State.LineModel.lineType = lineType; 
    shared->State.LineModel.linePos3 = linePos3;
  }
}

void LineModel::mix(Mixer &m) {
  INJECT_SHARED_bpos
  if (bpos == 7) {
    shared->GetUpdateBroadcaster()->subscribe(this);
  }
  if (bpos == 0) {
    INJECT_SHARED_buf
    INJECT_SHARED_pos
    INJECT_SHARED_c1
    uint64_t col = pos - nl1;
    const uint8_t cAbove = buf[nl2 + col];
    const uint8_t pCAbove = buf[nl2 + col - 1];

    const auto isNewLineStart = col == 0 && nl2 > 0;
    const auto isPrevCharMatchAbove = c1 == pCAbove && col != 0 && nl2 != 0;
    const uint32_t aboveCtx = cAbove << 1U | uint32_t(isPrevCharMatchAbove);
    if (isNewLineStart) {
      lineMatch = 0; //first char not yet known = nothing to match
    }
    else if (lineMatch >= 0 && isPrevCharMatchAbove) {
      lineMatch = min(lineMatch + 1, maxLineMatch); //match continues
    }
    else {
      lineMatch = -1; //match does not continue
    }


    uint64_t i = 0;
    const uint8_t RH = CM_USE_RUN_STATS | CM_USE_BYTE_HISTORY;

    cm.set(RH, hash(++i, line0));
    if (lineType != 1) { // not [a-zA-Z]
      cm.set(RH, hash(++i, aboveCtx, c1));
      cm.set(RH, hash(++i, col << 9 | aboveCtx, c1));
      cm.set(RH, hash(++i, nl1 - nl2, col, aboveCtx));
      cm.set(RH, hash(++i, cAbove, lineMatch + 1));
    }
    else {
      cm.skipn(RH, 4); 
      i += 4;
    }

    INJECT_SHARED_c4
    uint32_t lpfc = linePos3 << 2 | lineType;
    cm.set(RH, hash(++i, lpfc));
    cm.set(RH, hash(++i, lpfc, c1));
    cm.set(RH, hash(++i, lpfc, c4 & 0xffff));
    cm.set(RH, hash(++i, lpfc, c4 & 0xffffff));
    if (linePos3 == 0) {
      cm.set(RH, hash(++i, lpfc));
      cm.skip(RH); i++;
      cm.skip(RH); i++;
    }
    else if (linePos3 == 1) {
      cm.skip(RH); i++;
      cm.set(RH, hash(++i, lpfc << 8 | c1));
      cm.skip(RH); i++;
    }
    else { //if (linePos3 == 2)
      cm.skip(RH); i++;
      cm.skip(RH); i++;
      cm.set(RH, hash(++i, c4 & 0xffff, lpfc));
    }

    // modeling line content per column (and NEW_LINE in some extent)
    cm.set(RH, hash(++i, lineType, col));
    cm.set(RH, hash(++i, lineType, col, c1));
    cm.set(RH, hash(++i, lineType, col, c4));

    INJECT_SHARED_c8
    cm.set(RH, hash(++i, lineType, col, c8, buf(9) << 16 | buf(10) << 8 | buf(11)));

    cm.set(RH, hash(++i, lineType)); // order 0 context in paragraphs like this

    assert(i == nCM);
  }
  cm.mix(m);

  const uint32_t order = max(0, cm.order - (nCM - 15));
  assert(order <= 15);

  m.set(lineType + ((shared->State.order << 3) | (order >> 1)) * 3, 3 * 16 * 8);
  m.set(linePos3 * 3 + lineType, 3 * 3);

}


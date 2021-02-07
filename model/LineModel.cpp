#include "LineModel.hpp"


LineModel::LineModel(Shared* const sh, const uint64_t size) : 
  shared(sh),
  cm(sh, size, nCM, 64)
{}

void LineModel::mix(Mixer &m) {
  INJECT_SHARED_bpos
  if (bpos == 0) {
    INJECT_SHARED_pos
    INJECT_SHARED_c1

    uint8_t g = c1;
    if (g >= 128) {
      //utf8 code points (weak context)
      if ((g & 0xf8u) == 0xf0) g = 1;
      else if ((g & 0xf0u) == 0xe0) g = 2;
      else if ((g & 0xe0u) == 0xc0) g = 3;
      else if ((g & 0xc0u) == 0x80) g = 4; //the rest of the values
      else if (g == 0xff) g = 5;
      else g = c1 & 0xf0u;
    }
    else if (g >= '0' && g <= '9') g = '0';
    else if (g >= 'a' && g <= 'z') g = 'a';
    else if (g >= 'A' && g <= 'Z') g = 'A';
    groups = groups << 8 | g;

    const uint8_t RH = CM_USE_RUN_STATS | CM_USE_BYTE_HISTORY;
    const auto isNewline = c1 == NEW_LINE || c1 == 0;
    if (isNewline) { // a new line has just started (or: zero in asciiz or in binary data)
      nl2 = nl1;
      nl1 = pos;
      line0 = 0;
      groups = 0;
      firstChar = 0;
    }
    else {
      line0 = combine64(line0, c1);
    }

    uint64_t col = pos - nl1;
    if (col == 1) {
      if ('A' <= c1 && c1 <= 'Z')
        firstChar = 1;
      else if ('a' <= c1 && c1 <= 'z')
        firstChar = 1;
      else 
        firstChar = 2;

    }
    INJECT_SHARED_buf
    const uint8_t cAbove = buf[nl2 + col];
    const uint8_t pCAbove = buf[nl2 + col - 1];

    const auto isNewLineStart = col == 0 && nl2 > 0;
    const auto isPrevCharMatchAbove = c1 == pCAbove && col != 0 && nl2 != 0;
    const uint32_t aboveCtx = cAbove << 1U | uint32_t(isPrevCharMatchAbove);
    if (isNewLineStart) {
      lineMatch = 0; //first char not yet known = nothing to match
      groups = 0;
    }
    else if (lineMatch >= 0 && isPrevCharMatchAbove) {
      lineMatch = min(lineMatch + 1, maxLineMatch); //match continues
    }
    else {
      lineMatch = -1; //match does not continue
    }

    uint64_t i = 0;

    cm.set(RH, hash(++i, line0));

    // context: matches with the previous line
    if (firstChar != 1 && lineMatch >= 0) { // not [a-zA-Z]
      cm.set(RH, hash(++i, cAbove, lineMatch));
    }
    else {
      cm.skip(RH); i++;
    }

    if (firstChar != 1) { // not [a-zA-Z]
      cm.set(RH, hash(++i, aboveCtx, c1));
      cm.set(RH, hash(++i, col << 9 | aboveCtx, c1));
      const int lineLength = nl1 - nl2;
      cm.set(RH, hash(++i, nl1 - nl2, col, aboveCtx));
      cm.set(RH, hash(++i, nl1 - nl2, col, aboveCtx, c1));
      cm.set(RH, hash(++i, groups & 0xFF));
      cm.set(RH, hash(++i, groups & 0xFFFF));
    }
    else {
      cm.skipn(RH, 6); 
      i += 6;
    }

    // modeling line content per column (and NEW_LINE is some extent)
    cm.set(RH, hash(++i, col));
    cm.set(RH, hash(++i, col, c1));

    if (firstChar == 1) { // [a-zA-Z]
      INJECT_SHARED_c4
      cm.set(RH, hash(++i, col, c4));
      INJECT_SHARED_c8
      cm.set(RH, hash(++i, col, c4, c8));
    }
    else {
      cm.skipn(RH, 2);
      i += 2;
    }

    cm.set(RH, hash(++i, nl1)); // chars occurring in this paragraph (order 0)
    cm.set(RH, hash(++i, firstChar)); // order 0 context in paragraphs like this

    assert(i == nCM);
  }
  cm.mix(m);
  uint32_t order = min(cm.order, 15);
  
  m.set(firstChar + ((shared->State.order << 3) | (order >> 1)) * 3, 3 * 16 * 8);
  m.set(groups & 0xff, 256);

  shared->State.LineModel.firstLetter = firstChar;
}


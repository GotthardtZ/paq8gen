#include "LineModel.hpp"


LineModel::LineModel(Shared* const sh, const uint64_t size) : 
  shared(sh),
  cm(sh, size, nCM, 74)
{}

void LineModel::mix(Mixer &m) {
  INJECT_SHARED_bpos
  if (bpos == 0) {
    INJECT_SHARED_pos
    INJECT_SHARED_c1
    const uint8_t RH = CM_USE_RUN_STATS | CM_USE_BYTE_HISTORY;
    const auto isNewline = c1 == NEW_LINE || c1 == 0;
    if (isNewline) { // a new line has just started (or: zero in asciiz or in binary data)
      nl1 = pos;
      firstChar = 256;
    }

    uint64_t col = pos - nl1;
    if (col == 1) {
      firstChar = c1 >= 'A' && c1 <= 'Z' ? 'A' : c1;
    }

    uint64_t i = 0;

    // modeling line content per column
    cm.set(RH, hash(++i, col));
    cm.set(RH, hash(++i, col, c1));

    // content of lines, paragraphs
    cm.set(RH, hash(++i, nl1)); //chars occurring in this paragraph (order 0)
    cm.set(RH, hash(++i, firstChar)); //chars occurring in a paragraph that began with firstChar (order 0)
    assert(i == nCM);
  }
  cm.mix(m);
}

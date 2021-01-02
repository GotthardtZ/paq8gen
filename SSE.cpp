#include "SSE.hpp"

SSE::SSE(Shared* const sh) : shared(sh),
  Text {
    { /*APM:*/  {sh,0x10000,24}, {sh,0x10000,24}, {sh,0x10000,24}, {sh,0x10000,24}}, /* APM: contexts, steps */
    { /*APM1:*/ {sh,0x10000,7}, {sh,0x10000,6}, {sh,0x10000,6}} /* APM1: contexts, rate */
  },
  Generic {
    /*APM1:*/ {{sh,0x2000,7}, {sh,0x10000,7}, {sh,0x10000,7}, {sh,0x10000,7}, {sh,0x10000,7}, {sh,0x10000,7}, {sh,0x10000,7}}
  }
{}
auto SSE::p(int pr0) -> int {
  INJECT_SHARED_c0
  INJECT_SHARED_bpos
  INJECT_SHARED_c4
  INJECT_SHARED_blockPos
  int pr = 0;
  int pr1 = 0;
  int pr2 = 0;
  int pr3 = 0;
  INJECT_SHARED_blockType
    switch( blockType ) {
    case TEXT:
    case TEXT_EOL: {
      int limit = 0x3FFU >> (static_cast<int>(blockPos < 0xFFF) * 2);
      pr = Text.APMs[0].p(pr0, (c0 << 8U) | (shared->State.Text.mask & 0xFU) | ((shared->State.misses & 0xFU) << 4U), limit);
      pr1 = Text.APMs[1].p(pr0, finalize64(hash(bpos, shared->State.misses & 3U, c4 & 0xffffU, shared->State.Text.mask >> 4U), 16), limit);
      pr2 = Text.APMs[2].p(pr0, finalize64(hash(c0, shared->State.Match.expectedByte, shared->State.Match.length3), 16), limit);
      pr3 = Text.APMs[3].p(pr0, finalize64(hash(c0, c4 & 0xffffU, shared->State.Text.firstLetter), 16), limit);

      pr0 = (pr0 + pr1 + pr2 + pr3 + 2) >> 2U;

      pr1 = Text.APM1s[0].p(pr0, finalize64(hash(shared->State.Match.expectedByte, shared->State.Match.length3, c4 & 0xffU), 16));
      pr2 = Text.APM1s[1].p(pr, finalize64(hash(c0, c4 & 0x00ffffffU), 16));
      pr3 = Text.APM1s[2].p(pr, finalize64(hash(c0, c4 & 0xffffff00U), 16));

      pr = (pr + pr1 + pr2 + pr3 + 2) >> 2U;
      pr = (pr + pr0 + 1) >> 1U;
      break;
    }
    default: {
      pr = Generic.APM1s[0].p(pr0, (shared->State.Match.length3) << 11U | c0 << 3U | (shared->State.misses & 0x7U));
      const uint16_t ctx1 = c0 | (c4 & 0xffU) << 8U;
      const uint16_t ctx2 = c0 ^finalize64(hash(c4 & 0xffffU), 16);
      const uint16_t ctx3 = c0 ^finalize64(hash(c4 & 0xffffffU), 16);
      pr1 = Generic.APM1s[1].p(pr0, ctx1);
      pr2 = Generic.APM1s[2].p(pr0, ctx2);
      pr3 = Generic.APM1s[3].p(pr0, ctx3);

      pr0 = (pr0 + pr1 + pr2 + pr3 + 2) >> 2U;
      pr1 = Generic.APM1s[4].p(pr, (shared->State.Match.expectedByte << 8U) | (c4 & 0xffu));
      pr2 = Generic.APM1s[5].p(pr, ctx2);
      pr3 = Generic.APM1s[6].p(pr, ctx3);

      pr = (pr + pr1 + pr2 + pr3 + 2) >> 2U;
      pr = (pr + pr0 + 1) >> 1U;
    }
  }
  return pr;
}

#include "SSE.hpp"

SSE::SSE(Shared* const sh) : shared(sh), apm(sh, 1),
  /* APM: contexts, steps */
  APMs{{sh,0x10000,24}, {sh,0x10000,24}, {sh,0x10000,24}, {sh,0x10000,24}}, 
  /* APM1: contexts, rate */
  APM1s {{sh, 0x10000, 7}, { sh,0x10000,6 }, { sh,0x10000,6 }}
{}

auto SSE::p(int pr) -> int {
  INJECT_SHARED_c0
  INJECT_SHARED_bpos
  INJECT_SHARED_c4
  INJECT_SHARED_blockPos
  int pr0 = 0;
  int pr1 = 0;
  int pr2 = 0;
  int pr3 = 0;

  const uint32_t lineMode = shared->State.LineModel.firstLetter; //0..2
  const uint32_t expectedByte = shared->State.Match.expectedByte; //8 bit
  const uint32_t matchMode = shared->State.Match.length3; // 2 bit

  int limit = 0x3FFU >> (static_cast<int>(blockPos < 0xFFF) * 2);

  pr0 = APMs[0].p(pr, (c0 << 8) | lineMode, limit);
  pr1 = APMs[1].p(pr, finalize64(hash(bpos, lineMode, c4 & 0xffff, 0 >> 4), 16), limit);
  pr2 = APMs[2].p(pr, finalize64(hash(c0, expectedByte, matchMode), 16), limit);
  pr3 = APMs[3].p(pr, finalize64(hash(c0, c4 & 0xffff, lineMode), 16), limit);

  int prA = (pr + pr1 + pr2 + pr3 + 2) >> 2;

  pr1 = APM1s[0].p(pr0, finalize64(hash(c0, expectedByte, matchMode, c4 & 0xff, lineMode), 16));
  pr2 = APM1s[1].p(pr0, finalize64(hash(c0, c4 & 0x00ffffff, lineMode), 16));
  pr3 = APM1s[2].p(pr0, finalize64(hash(c0, (c4>>8)&0xffffff, lineMode), 16));

  int prB = (pr0 + pr1 + pr2 + pr3 + 2) >> 2;
  pr = (prA + prB + 1) >> 1;

  pr = apm.p(pr,0);

  return pr;
}

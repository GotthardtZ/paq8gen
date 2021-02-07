#include "SSE.hpp"

SSE::SSE(Shared* const sh) : shared(sh),
/* APM: contexts, steps */
  APMs{{sh,256*3,20}, {sh,0x10000,20}, {sh,0x10000,20}, {sh,0x10000,20}}, 
  /* APM1: contexts, rate */
  APM1s {{sh, 0x10000, 7}, { sh,0x10000,7 }, { sh,0x10000,7 }},
  APMPostA(sh, 1),
  APMPostB(sh, 1)
{}

uint32_t SSE::p(uint32_t pr_orig) {
  INJECT_SHARED_c0
  INJECT_SHARED_bpos
  INJECT_SHARED_c4

  const uint32_t lineMode = shared->State.LineModel.firstLetter; //0..2
  const uint32_t expectedByte = shared->State.Match.expectedByte; //8 bit
  const uint32_t matchLengthCtx = shared->State.Match.lengthCtx; // 2 bit

  uint32_t pr0 = APMs[0].p(pr_orig, c0 | lineMode<<8, 1023);
  uint32_t pr1 = APMs[1].p(pr_orig, finalize64(hash(bpos, c4 & 0xffffff, lineMode), 16), 1023);
  uint32_t pr2 = APMs[2].p(pr_orig, finalize64(hash(c0, expectedByte, matchLengthCtx), 16), 1023);
  uint32_t pr3 = APMs[3].p(pr_orig, finalize64(hash(c0, c4 & 0xffff, lineMode), 16), 1023);

  uint32_t prA = (pr_orig + pr1 + pr2 + pr3 + 2) >> 2;

  uint32_t pr4 = APM1s[0].p(pr0, finalize64(hash(c0, expectedByte, matchLengthCtx, c4 & 0xff, lineMode), 16));
  uint32_t pr5 = APM1s[1].p(pr0, finalize64(hash(c0, c4 & 0xffffff, lineMode), 16));
  uint32_t pr6 = APM1s[2].p(pr0, finalize64(hash(c0, (c4>>8)&0xffffff, lineMode), 16));

  uint32_t prB = (pr0 + pr4 + pr5 + pr6 + 2) >> 2;

  uint32_t pr = (APMPostA.p(prA, 0) + APMPostB.p(prB, 0) + 1) >> 1;

  return pr;
}

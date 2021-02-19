#include "SSE.hpp"

SSE::SSE(Shared* const sh) : shared(sh),
/* APM: contexts, steps */
  APMs{{sh,256*3,20}, {sh,0x10000,20}, {sh,0x10000,20}, {sh,0x10000,20} },
  /* APM1: contexts, rate */
  APM1s {{sh, 0x10000, 7}, { sh,0x10000,7 }, { sh,0x10000,7 }},
  APMPost0(sh, 8),
  APMPost1(sh, 8),
  APMPostA(sh, 8),
  APMPostB(sh, 8)
{}

uint32_t SSE::p(uint32_t pr_orig) {
  INJECT_SHARED_c0
  INJECT_SHARED_bpos
  INJECT_SHARED_c4

  const uint32_t lineType = shared->State.LineModel.lineType; //0..2
  assert(lineType <= 2);
  const uint32_t expectedByte = shared->State.Match.expectedByte; //8 bit
  assert(expectedByte <= 255);
  const uint32_t matchthCtx = shared->State.Match.matchCtx; //0-19
  assert(matchthCtx <= 19);

  uint32_t pr0 = APMs[0].p(pr_orig, c0 | lineType <<8, 1023);
  uint32_t pr1 = APMs[1].p(pr_orig, finalize64(hash(bpos, c4 & 0xffffff, lineType), 16), 1023);
  uint32_t pr2 = APMs[2].p(pr_orig, finalize64(hash(c0, expectedByte, matchthCtx), 16), 1023);
  uint32_t pr3 = APMs[3].p(pr_orig, finalize64(hash(c0, c4 & 0xffff, lineType), 16), 1023);

  uint32_t prA = (pr_orig + pr1 + pr2 + pr3 + 2) >> 2;

  uint32_t pr4 = APM1s[0].p(prA, finalize64(hash(c0, expectedByte, matchthCtx, c4 & 0xff, lineType), 16));
  uint32_t pr5 = APM1s[1].p(prA, finalize64(hash(c0, c4 & 0xffffff, lineType), 16));
  uint32_t pr6 = APM1s[2].p(prA, finalize64(hash(c0, (c4>>8)&0xffffff, lineType), 16));
  
  uint32_t prB = (prA + pr4 + pr5 + pr6 + 2) >> 2;

  uint32_t pr =
    (
      APMPost1.p(pr_orig, bpos) +
      APMPost0.p(pr0, bpos) +
      APMPostA.p(prA, bpos) +
      APMPostB.p(prB, bpos) +
      2
    ) >> 2;
  
  return pr;
}

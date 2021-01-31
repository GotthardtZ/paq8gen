#include "SSE.hpp"

SSE::SSE(Shared* const sh) : shared(sh), apmPost(sh, 1),
  /* APM: contexts, steps */
  APMs{{sh,256*3,20}, {sh,0x10000,20}, {sh,0x10000,20}, {sh,0x10000,20}}, 
  /* APM1: contexts, rate */
  APM1s {{sh, 0x10000, 7}, { sh,0x10000,7 }, { sh,0x10000,7 }}
{}

auto SSE::p(int pr) -> int {
  INJECT_SHARED_c0
  INJECT_SHARED_bpos
  INJECT_SHARED_c4
  int pr0 = 0;
  int pr1 = 0;
  int pr2 = 0;
  int pr3 = 0;

  const uint32_t lineMode = shared->State.LineModel.firstLetter; //0..2
  const uint32_t expectedByte = shared->State.Match.expectedByte; //8 bit
  const uint32_t matchMode = shared->State.Match.length3; // 2 bit

  pr0 = APMs[0].p(pr, c0 | lineMode<<8, 1023);
  pr1 = APMs[1].p(pr, finalize64(hash(bpos, c4 & 0xffffff, lineMode), 16), 1023);
  pr2 = APMs[2].p(pr, finalize64(hash(c0, expectedByte, matchMode), 16), 1023);
  pr3 = APMs[3].p(pr, finalize64(hash(c0, c4 & 0xffff, lineMode), 16), 1023);

  int prA = (pr + pr1 + pr2 + pr3 + 2) >> 2;

  pr1 = APM1s[0].p(pr0, finalize64(hash(c0, expectedByte, matchMode, c4 & 0xff, lineMode), 16));
  pr2 = APM1s[1].p(pr0, finalize64(hash(c0, c4 & 0xffffff, lineMode), 16));
  pr3 = APM1s[2].p(pr0, finalize64(hash(c0, (c4>>8)&0xffffff, lineMode), 16));

  int prB = (pr0 + pr1 + pr2 + pr3 + 2) >> 2;
  pr = (prA + prB + 1) >> 1;

  pr = apmPost.p(pr,0);

  return pr;
}

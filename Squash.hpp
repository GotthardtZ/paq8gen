#ifndef PAQ8GEN_SQUASH_HPP
#define PAQ8GEN_SQUASH_HPP

/**
 * return p = 1/(1 + exp(-d)), d scaled by 8 bits, p scaled by 12 bits
 * @param d
 * @return
 */
constexpr auto squash(int d) -> int {
  constexpr int t[33] = {1, 2, 3, 6, 10, 16, 27, 45, 73, 120, 194, 310, 488, 747, 1101, 1546, 2047, 2549, 2994, 3348, 3607, 3785, 3901,
                         3975, 4022, 4050, 4068, 4079, 4085, 4089, 4092, 4093, 4094};
  if( d > 2047 ) {
    return 4095;
  }
  if( d < -2047 ) {
    return 0;
  }
  const int w = d & 127U;
  d = (d >> 7U) + 16;
  return (t[d] * (128 - w) + t[(d + 1)] * w + 64) >> 7U;
}

#endif //PAQ8GEN_SQUASH_HPP

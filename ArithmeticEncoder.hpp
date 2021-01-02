#ifndef PAQ8GEN_ARITHMETICENCODER_HPP
#define PAQ8GEN_ARITHMETICENCODER_HPP

#include "file/FileDisk.hpp"

class ArithmeticEncoder {
public:
  ArithmeticEncoder(File* archive);
  uint32_t x1, x2; /**< Range, initially [0, 1), scaled by 2^32 */
  uint32_t x; /**< Decompress mode: last 4 input bytes of archive */
  File *archive; /**< Compressed data file */

  void prefetch();
  void flush();
  void encodeBit(int p, int bit);
  int decodeBit(int p);
};

#endif //PAQ8GEN_ARITHMETICENCODER_HPP

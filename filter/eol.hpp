#ifndef PAQ8GEN_EOL_HPP
#define PAQ8GEN_EOL_HPP

#include "../file/File.hpp"
#include "../Encoder.hpp"
#include "Filter.hpp"
#include <cstdint>

/**
 * End of line transform
 */
class EolFilter : public Filter {
public:
    void encode(File *in, File *out, uint64_t size, int /*info*/, int & /*headerSize*/) override {
      uint8_t b = 0;
      uint8_t pB = 0;
      for( int i = 0; i < static_cast<int>(size); i++ ) {
        b = in->getchar();
        if( pB == CARRIAGE_RETURN && b != NEW_LINE ) {
          out->putChar(pB);
        }
        if( b != CARRIAGE_RETURN ) {
          out->putChar(b);
        }
        pB = b;
      }
      if( b == CARRIAGE_RETURN ) {
        out->putChar(b);
      }
    }

    auto decode(File * /*in*/, File *out, FMode fMode, uint64_t size, uint64_t &diffFound) -> uint64_t override {
      uint8_t b = 0;
      uint64_t count = 0;
      for( int i = 0; i < static_cast<int>(size); i++, count++ ) {
        if((b = encoder->decompressByte()) == NEW_LINE ) {
          if( fMode == FDECOMPRESS ) {
            out->putChar(CARRIAGE_RETURN);
          } else if( fMode == FCOMPARE ) {
            if( out->getchar() != CARRIAGE_RETURN && (diffFound == 0u)) {
              diffFound = size - i;
              break;
            }
          }
          count++;
        }
        if( fMode == FDECOMPRESS ) {
          out->putChar(b);
        } else if( fMode == FCOMPARE ) {
          if( b != out->getchar() && (diffFound == 0u)) {
            diffFound = size - i;
            break;
          }
        }
#ifndef CHALLENGE
        if( fMode == FDECOMPRESS && ((i & 0xFFFU) == 0u)) {
          encoder->printStatus();
        }
#endif
      }
      return count;
    }
};

#endif //PAQ8GEN_EOL_HPP

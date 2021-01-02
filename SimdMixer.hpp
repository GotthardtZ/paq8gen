#ifndef PAQ8GEN_SIMDMIXER_HPP
#define PAQ8GEN_SIMDMIXER_HPP

#include "UpdateBroadcaster.hpp"
#include "BitCount.hpp"
#include "Ilog.hpp"
#include "Mixer.hpp"
#include "Squash.hpp"

template<SIMD simd>
class SIMDMixer : public Mixer {
private:
    SIMDMixer *mp; /**< points to a Mixer to combine results */

    /**
     * Define padding requirements.
     */
    [[nodiscard]] constexpr inline auto simdWidth() const -> int {
      if( simd == SIMD_AVX2 ) {
        return 32 / sizeof(short); // 256 bit (32 byte) data size
      }
      else if( simd == SIMD_SSE2 || simd == SIMD_SSSE3 || simd == SIMD_NEON ) {
        return 16 / sizeof(short); // 128 bit (16 byte) data size
      }
      else if( simd == SIMD_NONE ) {
        return 4 / sizeof(short); // Processes 2 shorts at once -> width is 4 bytes
      }
      assert(false);
    }

public:
    SIMDMixer(const Shared* const sh, const int n, const int m, const int s) : Mixer(sh, ((n + (simdWidth() - 1)) & -(simdWidth())), m, s) {
#ifdef VERBOSE
      printf("Created SIMDMixer with n = %d, m = %d, s = %d\n", n, m, s);
#endif
      assert((this->n & (simdWidth() - 1)) == 0);
      assert(this->m > 0);
      assert(this->s > 0);
      mp = (s > 1) ? new SIMDMixer<simd>(sh, s + (((sh->options & OPTION_LSTM) != 0u) ? 1 : 0), 1, 1) : nullptr;
    }

    ~SIMDMixer() override {
      delete mp;
    }

    void setScaleFactor(const int sf0, const int sf1) override {
      scaleFactor = sf0;
      if( mp ) {
        mp->setScaleFactor(sf1, 0);
      }
    }

    void promote(int x) override {
      if (mp != nullptr)
        mp->add(x);
    }

    /**
     * Adjust weights to minimize coding cost of last prediction.
     * Trains the network where the expected output is the last bit (in the shared variable y).
     */
    void update() override {
      INJECT_SHARED_y
      const int target = y << 12;
      if( nx > 0 ) {
        for( uint64_t i = 0; i < numContexts; ++i ) {
          if (cxt[i] != UINT32_MAX) {
            const int err = target - pr[i];
            if (simd == SIMD_NONE) {
              trainSimdNone(&tx[0], &wx[cxt[i] * n], nx, err * rates[i]);
            }
            else if (simd == SIMD_SSE2 || simd == SIMD_SSSE3) {
              trainSimdSse2(&tx[0], &wx[cxt[i] * n], nx, err * rates[i]);
            }
            else if (simd == SIMD_AVX2) {
              trainSimdAvx2(&tx[0], &wx[cxt[i] * n], nx, err * rates[i]);
            }
            else if (simd == SIMD_NEON) {
              trainSimdNeon(&tx[0], &wx[cxt[i] * n], nx, err * rates[i]);
            }
            if ((shared->options & OPTION_ADAPTIVE) != 0) {
              const uint32_t logErr = min(0xF, ilog2(abs(err)));
              info[i].sum -= square(info[i].data[1] >> 28);
              info[i].data[1] <<= 4;
              info[i].data[1] |= info[i].data[0] >> 28;
              info[i].data[0] <<= 4;
              info[i].data[0] |= logErr;
              info[i].sum += square(logErr);
              info[i].collected += info[i].collected < 4096;
              info[i].mask <<= 1;
              info[i].mask |= (logErr <= ((info[i].data[0] >> 4) & 0xF));
              const uint32_t count = bitCount(info[i].mask);
              if (info[i].collected >= 64 && (info[i].sum > 1500 + uint32_t(rates[i]) * 64 || count < 9 || (info[i].mask & 0xFF) == 0)) {
                rates[i] = DEFAULT_LEARNING_RATE;
                memset(&info[i], 0, sizeof(ErrorInfo));
              }
              else if (info[i].collected == 4096 && info[i].sum >= 56 && info[i].sum <= 144 && count > 28 - uint32_t(rates[i]) &&
                ((info[i].mask & 0xFF) == 0xFF)) {
                rates[i] -= rates[i] > 2;
                info[i].reset();
              }
            }
          }
        }
      }
      reset();
    }

    /**
     * Predict next bit
     * @return prediction
     */
    auto p() -> int override {
      shared->GetUpdateBroadcaster()->subscribe(this);
      assert(scaleFactor > 0);
      //if(mp)printf("nx: %d, numContexts: %d, base: %d\n",nx, numContexts, base); //for debugging: how many inputs do we have?
      while( nx & (simdWidth() - 1)) {
        tx[nx++] = 0; // pad
      }
      if( mp ) { // combine outputs
        for( uint64_t i = 0; i < numContexts; ++i ) {
          int dp = 0;
          if (cxt[i] != UINT32_MAX) { // valid mixer context (not to skip)
            if (simd == SIMD_NONE) {
              dp = dotProductSimdNone(&tx[0], &wx[cxt[i] * n], nx);
            }
            else if (simd == SIMD_SSE2 || simd == SIMD_SSSE3) {
              dp = dotProductSimdSse2(&tx[0], &wx[cxt[i] * n], nx);
            }
            else if (simd == SIMD_AVX2) {
              dp = dotProductSimdAvx2(&tx[0], &wx[cxt[i] * n], nx);
            }
            else if (simd == SIMD_NEON) {
              dp = dotProductSimdNeon(&tx[0], &wx[cxt[i] * n], nx);
            }
            dp = (dp * scaleFactor) >> 16;
            if (dp < -2047) {
              dp = -2047;
            }
            else if (dp > 2047) {
              dp = 2047;
            }
          }
          mp->add(dp);
          pr[i] = squash(dp);
        }
        mp->set(0, 1);
        return mp->p();
      } // s=1 context
      int dp = 0;
      if( simd == SIMD_NONE ) {
        dp = dotProductSimdNone(&tx[0], &wx[cxt[0] * n], nx);
      }
      else if( simd == SIMD_SSE2 || simd == SIMD_SSSE3 ) {
        dp = dotProductSimdSse2(&tx[0], &wx[cxt[0] * n], nx);
      }
      else if( simd == SIMD_AVX2 ) {
        dp = dotProductSimdAvx2(&tx[0], &wx[cxt[0] * n], nx);
      }
      else if (simd == SIMD_NEON) {
        dp = dotProductSimdNeon(&tx[0], &wx[cxt[0] * n], nx);
      }
      dp = (dp * scaleFactor) >> 16;
      return pr[0] = squash(dp);
    }
};

#endif //PAQ8GEN_SIMDMIXER_HPP

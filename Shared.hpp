#ifndef PAQ8GEN_SHARED_HPP
#define PAQ8GEN_SHARED_HPP

#include "UpdateBroadcaster.hpp"
#include "RingBuffer.hpp"
#include <cstdint>

// helper #defines to access shared variables
#define INJECT_SHARED_buf   const RingBuffer<uint8_t> &buf=shared->buf;
#define INJECT_SHARED_pos   const uint32_t pos=shared->buf.getpos();
#define INJECT_SHARED_y     const uint8_t  y=shared->State.y;
#define INJECT_SHARED_c0    const uint8_t  c0=shared->State.c0;
#define INJECT_SHARED_c1    const uint8_t  c1=shared->State.c1;
#define INJECT_SHARED_bpos  const uint8_t  bpos=shared->State.bitPosition;
#define INJECT_SHARED_c4    const uint32_t c4=shared->State.c4;
#define INJECT_SHARED_c8    const uint32_t c8=shared->State.c8;

#define INJECT_SHARED_blockType const BlockType blockType=shared->State.blockType;
#define INJECT_SHARED_blockPos const uint32_t blockPos=shared->State.blockPos;

/**
 * Shared information by all the models and some other classes.
 */
struct Shared {
private:
    UpdateBroadcaster updateBroadcaster;
public:

    //Shared state and statistics (global)
    
    RingBuffer<uint8_t> buf; /**< Rotating input queue set by Predictor */
    SIMD chosenSimd = SIMD_NONE; /**< default value, will be overridden by the CPU dispatcher, and may be overridden from the command line */
    uint8_t level = 0; /**< level=0: no compression (only transformations), level=1..12 compress using less..more RAM */
    uint64_t mem = 0; /**< pre-calculated value of 65536 * 2^level */
    bool toScreen = true;

    struct {

      //
      // Global state, used by most models, updated after every bit in by update(y)
      // 

      uint8_t y = 0; /**< Last bit, 0 or 1 */
      uint8_t c0 = 1; /**< Last 0-7 bits of the partial byte with a leading 1 bit (1-255) */
      uint8_t c1 = 0; /**< Last whole byte, equals to c4&0xff or buf(1) */
      uint8_t bitPosition = 0; /**< Bits in c0 (0 to 7), in other words the position of the bit to be predicted (0=MSB) */
      uint32_t c4 = 0; /**< Last 4 whole bytes (buf(4)..buf(1)), packed.  Last byte is bits 0-7. */
      uint32_t c8 = 0; /**< Another 4 bytes (buf(8)..buf(5)) */

      BlockType blockType{}; //used by wordModel, recordModel, SSE stage
      uint32_t blockPos{}; //relative position in block, used by many models

      //
      // State and statistics per model - set by the individual models
      // Order in appearance: models may use information from models that appears above them
      //

      //MatchModel
      struct {
        uint8_t matchCtx;   //used by SSE stage
        uint8_t expectedByte; //used by SSE stage
      } Match{};

      //NormalModel
      int order{};
      uint64_t cxt[30+1]{}; // context hashes

      //LineModel
      struct {
        uint8_t lineType; //used by SSE stage
        uint8_t linePos3;
      } LineModel{};

    } State{};

    Shared() {
      toScreen = !isOutputRedirected();
    }
    void init(uint8_t level);
    void update(int y);
    void reset();
    UpdateBroadcaster *GetUpdateBroadcaster() const;
private:

    /**
     * Copy constructor is private so that it cannot be called
     */
    Shared(Shared const & /*unused*/) {}

    /**
     * Assignment operator is private so that it cannot be called
     */
    auto operator=(Shared const & /*unused*/) -> Shared & { return *this; }

    /**
     * Determine if output is redirected
     * @return
     */
    static auto isOutputRedirected() -> bool;

    static Shared *mPInstance;
};

#endif //PAQ8GEN_SHARED_HPP

#ifndef PAQ8GEN_MATCHMODEL_HPP
#define PAQ8GEN_MATCHMODEL_HPP

#include "../Shared.hpp"
#include "../ContextMap2.hpp"
#include "../HashElementForMatchPositions.hpp"
#include "../LargeStationaryMap.hpp"
#include "../StationaryMap.hpp"

/**
 * Predict the next bit based on a preceding long matching byte sequence
 *
 * This model monitors byte sequences and keeps their most recent positions in a hashtable.
 * When the current byte sequence matches an older sequence (having the same hash) the model predicts the forthcoming bits.
 */
class MatchModel : IPredictor {
private:
    static constexpr int nCM = 2;
    static constexpr int nST = 3;
    static constexpr int nLSM = 1;
    Shared * const shared;
    Array<HashElementForMatchPositions> hashtable;
    StateMap stateMaps[nST];
    ContextMap2 cm;
    LargeStationaryMap mapL;
    uint32_t ctx[nST] {0};
    const int hashBits;
    Ilog *ilog = &Ilog::getInstance();
#ifdef CHALLENGE
    Stretch * stretch = &Stretch::getInstance();
#endif
    static constexpr uint32_t MINLEN_RM = 5; //minimum length in recovery mode before we "fully recover"
    static constexpr uint32_t MINLEN_RM_LE = 0; //minimum length in recovery mode before we "fully recover" for Low Entropy content (0 = recover immediately)
    static constexpr int LEN1 = 12;
    static constexpr int LEN2 = 18;
    static constexpr int LEN3 = 24;
    static constexpr int LEN4 = 30;

    struct MatchInfo {
      uint32_t length = 0; /**< rebased length of match (length=1 represents the smallest accepted match length), or 0 if no match */
      uint32_t index = 0; /**< points to next byte of match in buf, 0 when there is no match */
      uint32_t lengthBak = 0; /**< allows match recovery after a 1-byte mismatch */
      uint32_t indexBak = 0;
      uint8_t expectedByte = 0; /**< prediction is based on this byte (buf[index]), valid only when length>0 */
      bool delta = false; /**< indicates that a match has just failed (delta mode) */

      bool isInNoMatchMode() {
        return length == 0 && !delta && lengthBak == 0;
      }

      bool isInPreRecoveryMode() {
        return length == 0 && !delta && lengthBak != 0;
      }

      bool isInRecoveryMode() {
        return length != 0 && lengthBak != 0;
      }

      uint32_t recoveryModePos() {
        assert(isInRecoveryMode()); //must be in recovery mode
        return length - lengthBak;
      }

      uint64_t prio() {
        return
          static_cast<uint64_t>(length != 0) << 49 | //normal mode (match)
          static_cast<uint64_t>(delta) << 48 | //delta mode
          static_cast<uint64_t>(delta ? lengthBak : length) << 32 | //the longer wins
          static_cast<uint64_t>(index); //the more recent wins
      }
      bool isBetterThan(MatchInfo* other) {
        return this->prio() > other->prio();
      }

      void update(Shared* shared) {
        if constexpr (false) {
          INJECT_SHARED_bpos
          INJECT_SHARED_blockPos
          printf("- pos %d %d  index %d  length %d  lengthBak %d  delta %d\n", blockPos, bpos, index, length, lengthBak, delta ? 1 : 0);
        }
        INJECT_SHARED_buf
        INJECT_SHARED_bpos
        if (length != 0) {
          const int expectedBit = (expectedByte >> ((8 - bpos) & 7)) & 1;
          INJECT_SHARED_y
          if (y != expectedBit) {
            if (isInRecoveryMode()) { // another mismatch in recovery mode -> give up
              lengthBak = 0;
              indexBak = 0;
            }
            else { //backup match information: maybe we can recover it just after this mismatch
              lengthBak = length;
              indexBak = index;
              delta = true; //enter into delta mode - for the remaining bits in this byte length will be 0; we will exit delta mode and enter into recovery mode on bpos==0
            }
            length = 0;
          }
        }

        if (bpos == 0) {

          // recover match after a 1-byte mismatch
          if (isInPreRecoveryMode()) { // just exited delta mode, so we have a backup
            //the match failed 2 bytes ago, we must increase indexBak by 2:
            indexBak++;
            if (lengthBak < 65535) {
              lengthBak++;
            }
            INJECT_SHARED_c1
            if (buf[indexBak] == c1) { // match continues -> recover
              length = lengthBak;
              index = indexBak;
            }
            else { // still mismatch
              lengthBak = indexBak = 0; // purge backup (give up)
            }
          }

          // extend current match
          if (length != 0) {
            index++;
            if (length < 65535) {
              length++;
            }
            if (isInRecoveryMode() && recoveryModePos() >= (shared->State.LineModel.lineType == 1 ? MINLEN_RM_LE : MINLEN_RM)) { // recovery seems to be successful and stable -> exit recovery mode
              lengthBak = indexBak = 0; // purge backup
            }
          }
          delta = false;
        }
        if constexpr(false) {
          INJECT_SHARED_bpos
          INJECT_SHARED_blockPos
          printf("  pos %d %d  index %d  length %d  lengthBak %d  delta %d\n", blockPos, bpos, index, length, lengthBak, delta ? 1 : 0);
        }

      }

      void registerMatch(const uint32_t pos, const uint32_t LEN) {
        assert(pos != 0);
        length = LEN - LEN1 + 1; // rebase
        index = pos;
        lengthBak = indexBak = 0;
        expectedByte = 0;
        delta = false;
      }

    };

    bool isMatch(const uint32_t pos, const uint32_t MINLEN) {
      INJECT_SHARED_buf
      for (uint32_t length = 1; length <= MINLEN; length++) {
        if (buf(length) != buf[pos - length])
          return false;
      }
      return true;
    }

    void AddCandidates(HashElementForMatchPositions* matches, uint32_t LEN) {
      uint32_t i = 0;
      INJECT_SHARED_pos
      while (numberOfActiveCandidates < N && i < HashElementForMatchPositions::N) {
        uint32_t matchpos = matches->matchPositions[i];
        if (matchpos == 0)
          break;
        if (isMatch(matchpos, LEN)) {
          bool isSame = false;
          //is this position already registered?
          for (uint32_t j = 0; j < numberOfActiveCandidates; j++) {
            MatchInfo* oldcandidate = &matchCandidates[j];
            if (isSame = oldcandidate->index == matchpos)
              break;
          }
          if (!isSame) { //don't register an already registered sequence
            matchCandidates[numberOfActiveCandidates].registerMatch(matchpos, LEN);
            numberOfActiveCandidates++;
          }
        }
        i++;
      }
    }

    static constexpr size_t N = 4; // maximum number of match candidates
    Array<MatchInfo> matchCandidates{ N };
    uint32_t numberOfActiveCandidates = 0;

public:
    static constexpr int MIXERINPUTS = 
      2 + // direct inputs based on expectedBit
      nCM * (ContextMap2::MIXERINPUTS + ContextMap2::MIXERINPUTS_RUN_STATS) + 
      nST * 2 +
      nLSM * LargeStationaryMap::MIXERINPUTS; // 21
    static constexpr int MIXERCONTEXTS = 20 + (4 * 8) + ((N + 1) * (N + 1) * 8);
    static constexpr int MIXERCONTEXTSETS = 3;
    MatchModel(Shared* const sh, const uint64_t hashtablesize, const uint64_t mapmemorysize);
    void update();
    void mix(Mixer &m);
};

#endif //PAQ8GEN_MATCHMODEL_HPP

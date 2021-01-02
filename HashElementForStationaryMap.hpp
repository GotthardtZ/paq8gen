#ifndef PAQ8GEN_HASHELEMENTFORSTATIONARYMAP_HPP
#define PAQ8GEN_HASHELEMENTFORSTATIONARYMAP_HPP

#include <cstdint>

struct HashElementForStationaryMap { // sizeof(HashElemetForStationaryMap) = 4

  uint32_t value;

  // priority for hash replacement strategy
  uint32_t prio() {
    return (value >> 16) + (value & 0xffff); // to be tuned
  }

};

#endif //PAQ8GEN_HASHELEMENTFORSTATIONARYMAP_HPP

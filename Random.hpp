#ifndef PAQ8GEN_RANDOM_HPP
#define PAQ8GEN_RANDOM_HPP

#include "Array.hpp"
#include <cstdint>

/**
 * 32-bit pseudo random number generator
 */
class Random {
    uint64_t _state;

public:
    Random();
    auto operator()(int numberOfBits) -> uint32_t;
};

#endif //PAQ8GEN_RANDOM_HPP

#ifndef PAQ8GEN_MIXERFACTORY_HPP
#define PAQ8GEN_MIXERFACTORY_HPP

#include "utils.hpp"
#include "Mixer.hpp"
#include "Shared.hpp"
#include "SimdMixer.hpp"

class MixerFactory {
public:
    auto createMixer(const Shared* const sh, int n, int m, int s) -> Mixer *;
};

#endif //PAQ8GEN_MIXERFACTORY_HPP

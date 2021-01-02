#ifndef PAQ8GEN_IACTIVATION_HPP
#define PAQ8GEN_IACTIVATION_HPP

#include "../utils.hpp"

class IActivation {
public:
  virtual ~IActivation() = default;
  virtual void Run(float* f, std::size_t const len) const = 0;
};

#endif //PAQ8GEN_IACTIVATION_HPP

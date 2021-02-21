#ifdef CHALLENGE
#include "Stretch.hpp"
Stretch *Stretch::instance = nullptr;


auto Stretch::getInstance() -> Stretch& {
  static Stretch instance;
  return instance;
}

Stretch *stretch = &Stretch::getInstance();

#endif
#include "ProgramChecker.hpp"

#ifndef CHALLENGE
ProgramChecker *ProgramChecker::instance = nullptr;

auto ProgramChecker::getInstance() -> ProgramChecker * {
  if( instance == nullptr ) {
    instance = new ProgramChecker();
  }
  return instance;
}

void ProgramChecker::alloc(uint64_t n) {
  memUsed += n;
  if( memUsed > maxMem ) {
    maxMem = memUsed;
  }
}

void ProgramChecker::free(uint64_t n) {
  assert(memUsed >= n);
  memUsed -= n;
}

auto ProgramChecker::getRuntime() const -> double {
  const std::chrono::time_point<std::chrono::high_resolution_clock> finishTime = std::chrono::high_resolution_clock::now();
  return std::chrono::duration<double>(finishTime - startTime).count();
}

void ProgramChecker::print() const {
  const double runtime = getRuntime();
  printf("Time %1.2f sec, used %" PRIu64 " MB (%" PRIu64 " bytes) of memory\n", runtime, maxMem >> 20U, maxMem);
}

ProgramChecker::~ProgramChecker() {
  assert(memUsed == 0); // We expect that all reserved memory is already properly freed
}

#endif
#include "Shared.hpp"

/*
 relationship between compression level, shared->mem and buf memory use 

 level   shared->mem    buf memory use (shared->mem * 8)
 -----   -----------    --------------
   1      0.125 MB              1 MB
   2      0.25	MB              2 MB
   3      0.5   MB              4 MB
   4      1.0   MB              8 MB
   5      2.0   MB             16 MB
   6      4.0   MB             32 MB
   7      8.0   MB             64 MB
   8     16.0   MB            128 MB
   9     32.0   MB            256 MB
  10     64.0   MB            512 MB
  11    128.0   MB           1024 MB
  12    256.0   MB           1024 MB

*/

void Shared::init(uint8_t level) {
  this->level = level;
  mem = UINT64_C(65536) << level;
  buf.setSize(static_cast<uint32_t>(min(mem * 8, UINT64_C(1) << 30))); /**< no reason to go over 1 GB */
  toScreen = !isOutputRedirected();
}

void Shared::update(int y) {
  State.y = y;
  State.c0 += State.c0 + y;
  State.bitPosition = (State.bitPosition + 1U) & 7U;
  if(State.bitPosition == 0 ) {
    State.c1 = State.c0;
    buf.add(State.c1);
    State.c8 = (State.c8 << 8U) | (State.c4 >> 24U);
    State.c4 = (State.c4 << 8U) | State.c0;
    State.c0 = 1;
  }
  
  // Broadcast to all current subscribers: y (and c0, c1, c4, etc) is known
  updateBroadcaster.broadcastUpdate();
}

void Shared::reset() {
  buf.reset();
  memset(&State, 0, sizeof(State));
  State.c0 = 1;
}

UpdateBroadcaster *Shared::GetUpdateBroadcaster() const {
  UpdateBroadcaster* updater = const_cast<UpdateBroadcaster*>(&updateBroadcaster);
  return updater;
}

auto Shared::isOutputRedirected() -> bool {
#ifdef WINDOWS
  DWORD FileType = GetFileType(GetStdHandle(STD_OUTPUT_HANDLE));
  return (FileType == FILE_TYPE_PIPE) || (FileType == FILE_TYPE_DISK);
#endif
#ifdef UNIX
  return isatty(fileno(stdout)) == 0;
#endif
}

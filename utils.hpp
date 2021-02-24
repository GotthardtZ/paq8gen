#ifndef PAQ8GEN_UTILS_HPP
#define PAQ8GEN_UTILS_HPP

#include <cstdint>
#include "BitCount.hpp"

static_assert(sizeof(short) == 2, "sizeof(short)");
static_assert(sizeof(int) == 4, "sizeof(int)");

//////////////////////// Target OS/Compiler ////////////////////////////////

#if defined(_WIN32) || defined(_MSC_VER)
#ifndef WINDOWS
#define WINDOWS  //to compile for Windows
#endif
#endif

#if defined(unix) || defined(__unix__) || defined(__unix) || defined(__APPLE__)
#ifndef UNIX
#define UNIX //to compile for Unix, Linux, Solaris, MacOS / Darwin, etc)
#endif
#endif

#if !defined(WINDOWS) && !defined(UNIX)
#error Unknown target system
#endif

// Floating point operations need IEEE compliance
// Do not use compiler optimization options such as the following:
// gcc : -ffast-math (and -Ofast, -funsafe-math-optimizations, -fno-rounding-math)
// vc++: /fp:fast
#if defined(__FAST_MATH__) || defined(_M_FP_FAST) // gcc vc++
#error Avoid using aggressive floating-point compiler optimization flags
#endif

#if defined(_MSC_VER)
#define ALWAYS_INLINE  __forceinline
#elif defined(__GNUC__)
#define ALWAYS_INLINE inline __attribute__((always_inline))
#else
#define ALWAYS_INLINE inline
#endif


#if defined(NDEBUG)
#if defined(_MSC_VER)
#define assume(cond) __assume(cond)
#else
#define assume(cond) do { if (!(cond)) __builtin_unreachable(); } while (0)
#endif
#else
#include <cassert>
#define assume(cond) assert(cond)
#endif

#include <algorithm>
#include "cstdio.hpp"
// Determining the proper printf() format specifier for 64 bit unsigned integers:
// - on Windows MSVC and MinGW-w64 use the MSVCRT runtime where it is "%I64u"
// - on Linux it is "%llu"
// The correct value is provided by the PRIu64 macro which is defined here:
#include <cinttypes> //PRIu64

// Platform-specific includes
#ifdef UNIX

#include <cerrno>  //errno
#include <climits> //PATH_MAX (for OSX)
#include <cstring> //strlen(), strcpy(), strcat(), strerror(), memset(), memcpy(), memmove()
#include <unistd.h> //isatty()

#else
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>   //CreateDirectoryW, CommandLineToArgvW, GetConsoleOutputCP, SetConsoleOutputCP
//GetCommandLineW, GetModuleFileNameW, GetStdHandle, GetTempFileName
//MultiByteToWideChar, WideCharToMultiByte,
//FileType, FILE_TYPE_PIPE, FILE_TYPE_DISK,
//uRetVal, DWORD, UINT, TRUE, MAX_PATH, CP_UTF8, etc.
#endif

typedef enum {
  SIMD_NONE, SIMD_SSE2, SIMD_SSSE3, SIMD_AVX2, SIMD_NEON
} SIMD;


static inline auto min(int a, int b) -> int { return std::min<int>(a, b); }

static inline auto min(uint32_t a, uint32_t b) -> uint32_t { return std::min<uint32_t>(a, b); }

static inline auto min(uint64_t a, uint64_t b) -> uint64_t { return std::min<uint64_t>(a, b); }

static inline auto max(int a, int b) -> int { return std::max<int>(a, b); }

template<typename T>
constexpr auto isPowerOf2(T x) -> bool {
  return ((x & (x - 1)) == 0);
}


#ifndef NDEBUG
#if defined(UNIX)
#include <execinfo.h>
#define BACKTRACE() \
  { \
    void *callstack[128]; \
    int frames  = backtrace(callstack, 128); \
    char **strs = backtrace_symbols(callstack, frames); \
    for (int i = 0; i < frames; ++i) { \
      printf("%s\n", strs[i]); \
    } \
    free(strs); \
  }
#else
// TODO: How to implement this on Windows?
#define BACKTRACE() \
  { }
#endif
#else
#define BACKTRACE()
#endif

// A basic exception class to let catch() in main() know
// that the exception was thrown intentionally.
class IntentionalException : public std::exception {};

// Error handler: print message if any, and exit
[[noreturn]] static void quit(const char *const message = nullptr) {
  if( message != nullptr ) {
    printf("\n%s", message);
  }
  printf("\n");
  throw IntentionalException();
}

typedef enum {
  DEFAULT = 0,
  FILECONTAINER,
  TEXT,
  TEXT_EOL,
  Count
} BlockType;

static inline auto hasRecursion(BlockType ft) -> bool {
  return ft == FILECONTAINER;
}

static inline auto hasInfo(BlockType ft) -> bool {
  return ft == Count;
}

static inline auto hasTransform(BlockType ft) -> bool {
  return ft == TEXT_EOL;
}

//////////////////// Cross-platform definitions /////////////////////////////////////

#ifdef _MSC_VER
#define fseeko(a, b, c) _fseeki64(a, b, c)
#define ftello(a) _ftelli64(a)
#else
#ifndef UNIX
#ifndef fseeko
#define fseeko(a, b, c) fseeko64(a, b, c)
#endif
#ifndef ftello
#define ftello(a) ftello64(a)
#endif
#endif
#endif

#ifdef WINDOWS
#define strcasecmp _stricmp
#endif

#if defined(__GNUC__) || defined(__clang__)
#define bswap(x) __builtin_bswap32(x)
#define bswap64(x) __builtin_bswap64(x)
#elif defined(_MSC_VER)
#define bswap(x) _byteswap_ulong(x)
#define bswap64(x) _byteswap_uint64(x)
#else
#define bswap(x) \
  +(((( x ) &0xff000000) >> 24) | +((( x ) &0x00ff0000) >> 8) | +((( x ) &0x0000ff00) << 8) | +((( x ) &0x000000ff) << 24))
#define bswap64(x) \
  +((x) >> 56) |
+   (((x)<<40) & 0x00FF000000000000) | \
+   (((x)<<24) & 0x0000FF0000000000) | \
+   (((x)<<8 ) & 0x000000FF00000000) | \
+   (((x)>>8 ) & 0x00000000FF000000) | \
+   (((x)>>24) & 0x0000000000FF0000) | \
+   (((x)>>40) & 0x000000000000FF00) | \
+   ((x) << 56))
#endif


#define TAB 0x09
#define NEW_LINE 0x0A
#define CARRIAGE_RETURN 0x0D
#define SPACE 0x20
#define QUOTE 0x22
#define APOSTROPHE 0x27

/**
 * Returns floor(log2(x)).
 * 0/1->0, 2->1, 3->1, 4->2 ..., 30->4,  31->4, 32->5,  33->5
 * @param x
 * @return floor(log2(x))
 */
static auto ilog2(uint32_t x) -> uint32_t {
#ifdef _MSC_VER
  DWORD tmp = 0;
  if (x != 0) {
    _BitScanReverse(&tmp, x);
  }
  return tmp;
#elif __GNUC__
  if( x != 0 ) {
    x = 31 - __builtin_clz(x);
  }
  return x;
#else
  //copy the leading "1" bit to its left (0x03000000 -> 0x03ffffff)
  x |= (x >> 1);
  x |= (x >> 2);
  x |= (x >> 4);
  x |= (x >> 8);
  x |= (x >> 16);
  //how many trailing bits do we have (except the first)?
  return BitCount(x >> 1);
#endif
}


#ifdef _MSC_VER
#include <intrin.h>
inline uint32_t clz(uint32_t x) {
  DWORD leading_zero;
  if (x != 0u) {
    _BitScanReverse(&leading_zero, x);
    return 31u - leading_zero;
  }
  return 32u;
}
#elif __GNUC__
inline uint32_t clz(uint32_t x) {
  if (x != 0u)
    return __builtin_clz(x);
  return 32u;
}
#else
inline uint32_t popcnt(uint32_t x) {
  x -= ((x >> 1) & 0x55555555);
  x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
  x = (((x >> 4) + x) & 0x0f0f0f0f);
  x += (x >> 8);
  x += (x >> 16);
  return x & 0x0000003f;
}
inline uint32_t clz(uint32_t x) {
  x |= (x >> 1);
  x |= (x >> 2);
  x |= (x >> 4);
  x |= (x >> 8);
  x |= (x >> 16);
  return 32u - popcnt(x);
}
#endif

#endif //PAQ8GEN_UTILS_HPP

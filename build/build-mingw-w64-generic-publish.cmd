@echo off
rem * This script builds a paq8gen executable for all x86/x64 CPUs - no questions asked
rem * Remark: SIMD-specific functions are dispatched at runtime to match the runtime processor. (See -march=nocona -mtune=generic below.)
rem * If the build fails see compiler errors in _error.txt

rem * Set your mingw-w64 path below
set path=%path%;c:/Program Files/mingw-w64/winlibs-x86_64-posix-seh-gcc-9.3.0-llvm-10.0.0-mingw-w64-7.0.0-r4/bin

rem * The following settings are for a release build.
rem * For a debug build remove -DNDEBUG to enable asserts and array bound checks and add -Wall to show compiler warnings.
set options=-DNDEBUG -O3 -m64 -march=nocona -mtune=generic -flto -fwhole-program -floop-strip-mine -funroll-loops -ftree-vectorize -fgcse-sm -falign-loops=16

del _error.txt  >nul 2>&1
del paq8gen.exe       >nul 2>&1

g++.exe -s -static -fno-rtti -std=gnu++1z %options% ../file/*.cpp ../filter/*.cpp ../model/*.cpp ../*.cpp -opaq8gen.exe    2>_error.txt
pause

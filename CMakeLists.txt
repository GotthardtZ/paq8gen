cmake_minimum_required(VERSION 3.10)
project(paq8gen)

include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)

option(NATIVECPU "Whether to build for your cpu (vs. the general public)" OFF)
option(NDEBUG "Whether to suppress asserts and array bound checks" ON)

if (NATIVECPU)
    add_definitions(-march=native -mtune=native)
else ()
    add_definitions(-march=nocona -mtune=generic)
endif (NATIVECPU)

if (NDEBUG)
    add_definitions(-DNDEBUG)
endif (NDEBUG)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "-O3")

add_executable(paq8gen ProgramChecker.cpp paq8gen.cpp  Random.cpp String.cpp Predictor.cpp Models.cpp APM1.cpp APMPost.cpp Ilog.cpp model/ContextModel.cpp SSE.cpp UpdateBroadcaster.cpp Shared.cpp file/File.cpp file/FileDisk.cpp file/FileName.cpp file/FileTmp.cpp model/MatchModel.cpp model/NormalModel.cpp model/LineModel.cpp StationaryMap.cpp StateTable.cpp StateMap.cpp MixerFactory.cpp Mixer.cpp ContextMap2.cpp APM.cpp AdaptiveMap.cpp filter/TextParserStateInfo.cpp Encoder.cpp ArithmeticEncoder.cpp LargeStationaryMap.cpp Transformations.cpp)

if (supported)
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET paq8gen PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else ()
    message(STATUS "IPO / LTO not supported: <${error}>")
endif ()
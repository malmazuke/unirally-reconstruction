cmake_minimum_required(VERSION 3.27)

# Laboratory infrastructure build. src/lab is a synthetic determinism probe
# used to validate the toolchain and report pipeline. It contains no
# recovered game code. src/core contains the M2-01 collision-sampling
# prerequisite; it is not yet an autonomous native game update.
project(unirally_lab VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(LAB_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(LAB_SANITIZERS "Enable address and undefined-behaviour sanitizers" OFF)
option(LAB_PROBE_FAILURES "Register deliberately failing and timing-out tests to exercise the report pipeline" OFF)
option(LAB_BUILD_APP "Build the SDL3 desktop frontend" OFF)

add_library(lab_options INTERFACE)
if(MSVC)
  target_compile_options(lab_options INTERFACE /W4 $<$<BOOL:${LAB_WARNINGS_AS_ERRORS}>:/WX>)
else()
  target_compile_options(lab_options INTERFACE
    -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
    $<$<BOOL:${LAB_WARNINGS_AS_ERRORS}>:-Werror>)
  if(LAB_SANITIZERS)
    target_compile_options(lab_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(lab_options INTERFACE -fsanitize=address,undefined)
  endif()
endif()

add_subdirectory(src/lab)
add_subdirectory(third_party/spc_dsp)
add_subdirectory(src/core)
add_subdirectory(src/app)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests/synthetic)
  add_subdirectory(tests/native)
  add_subdirectory(tests/app)
endif()

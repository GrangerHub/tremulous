
#   M""""""""M                                       dP                             #
#   Mmmm  mmmM                                       88                             #
#   MMMM  MMMM 88d888b. .d8888b. 88d8b.d8b. dP    dP 88 .d8888b. dP    dP .d8888b.  #
#   MMMM  MMMM 88'  `88 88ooood8 88'`88'`88 88    88 88 88'  `88 88    88 Y8ooooo.  #
#   MMMM  MMMM 88       88.  ... 88  88  88 88.  .88 88 88.  .88 88.  .88       88  #
#   MMMM  MMMM dP       `88888P' dP  dP  dP `88888P' dP `88888P' `88888P' `88888P'  #
#   MMMMMMMMMM                                                                      #


cmake_minimum_required(VERSION 3.10...4.0)
project(Tremulous C CXX)

# C/C++ language standard flags — GCC/Clang only via generator expressions.
# MSVC does not understand -std=gnu99 / -std=c++1y; it uses /std:c99 etc.
# (set automatically when CMAKE_C_STANDARD / CMAKE_CXX_STANDARD are used, or
# defaulted from the toolchain). The generator expression below expands to
# the -std flag only when the active compiler is GNU, Clang, or AppleClang.
add_compile_options(
    $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-std=gnu99>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-std=c++1y>
)

# MSVC: enable /permissive- (strict C++ conformance). Required because the
# codebase uses ISO C++ alternative operator tokens `and`/`or`/`not` as
# keywords (e.g. files.cpp:3569). Without /permissive- MSVC reports
# C2146 "syntax error: missing ')' before identifier 'and'".
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/permissive->)

# -fPIC is GCC/Clang-only and only relevant for Linux shared objects.
# MSVC position-independent code is the default for /MD and needs no flag.
# Keep the existing Linux gate, but restrict to GCC family too so a future
# Linux+MSVC (clang-cl) build does not get confused.
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    add_compile_options(
        $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-fPIC>
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fPIC>
    )
endif()

# Global include path — every target needs to resolve headers via their
# module-relative path (e.g. #include "qcommon/q_platform.h",
# #include "sys/sys_shared.h", #include "game/g_public.h"). Without this,
# MSVC builds fail with "Cannot open include file" because each
# add_subdirectory()'d CMakeLists only adds its OWN directory to the path.
include_directories(${CMAKE_SOURCE_DIR}/src)

# XXX This one doesn't work yet
option(USE_RENDERER_DLOPEN "" ON)

# These are not used at the moment 
if (APPLE OR WIN32)
  option(USE_INTERNAL_SDL2 "" ON)
else ()
  option(USE_INTERNAL_SDL2 "" OFF)
endif()

option(USE_INTERNAL_JPEG "" ON)
option(USE_OPENAL "" ON)
option(USE_OPENAL_DLOPEN "" ON)
option(USE_VOIP "" OFF)
option(USE_MUMBLE "" OFF)
option(USE_CODEC_OPUS "" ON)
option(USE_RESTCLIENT "" ON)

include(${CMAKE_SOURCE_DIR}/cmake/build_dir)
include(${CMAKE_SOURCE_DIR}/cmake/debug_cflags)

# builtins
# Upgraded to SDL3 (v3.5.0) from external source
# External paths require explicit binary directory
add_subdirectory(E:/code/2026/YOLO/quake-repos/libraries/SDL build-vs2022-x64/SDL3)

# Added libcurl (v8.22.0-DEV) from external source
# Configure with minimal dependencies for game use
set(CURL_DISABLE_TESTS ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_ALTSVC ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_HSTS ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_COOKIES OFF CACHE BOOL "" FORCE)
set(CURL_DISABLE_HTTP_AUTH OFF CACHE BOOL "" FORCE)
set(CURL_DISABLE_DOH ON CACHE BOOL "" FORCE)  # DNS-over-HTTPS not needed
set(CURL_DISABLE_CAACHE ON CACHE BOOL "" FORCE)  # Disable cache
set(CURL_DISABLE_WEBSOCKETS ON CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_CURL_MANUAL OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)  # Disable Libpsl dependency
set(CURL_ZLIB OFF CACHE BOOL "" FORCE)  # Disable zlib dependency (we have our own)
set(CURL_BROTLI OFF CACHE BOOL "" FORCE)  # Disable Brotli
set(CURL_ZSTD OFF CACHE BOOL "" FORCE)  # Disable Zstd
set(CURL_NGHTTP2 OFF CACHE BOOL "" FORCE)  # Disable HTTP/2
set(CURL_USE_LIBIDN2 OFF CACHE BOOL "" FORCE)  # Disable libidn2
add_subdirectory(E:/code/2026/YOLO/quake-repos/libraries/curl build-vs2022-x64/curl)

add_subdirectory(external/lua-5.3.3)
add_subdirectory(external/nettle-3.3)
add_subdirectory(external/zlib)
add_subdirectory(external/jpeg-8c)
add_subdirectory(external/restclient)

# audio
add_subdirectory(external/opus-1.1.4)
add_subdirectory(external/opusfile-0.8)
add_subdirectory(external/libogg-1.3.2)
add_subdirectory(external/libvorbis-1.3.5)

# engine
add_subdirectory(src/script)
add_subdirectory(src/renderercommon)
add_subdirectory(src/renderergl1)
add_subdirectory(src/renderergl2)

#         .                    .
#   ___   |   `   ___  , __   _/_
# .'   `  |   | .'   ` |'  `.  |
# |       |   | |----' |    |  |
#  `._.' /\__ / `.___, /    |  \__/
#

add_subdirectory(src/client)

#   ____   ___  .___  _   __   ___  .___
#  (     .'   ` /   \ |   /  .'   ` /   \
#  `--.  |----' |   ' `  /   |----' |   '
# \___.' `.___, /      \/    `.___, /
#
 
add_subdirectory(src/server)

#   ___
# .'   \  .___    ___  , __     ___.   ___  .___
# |       /   \  /   ` |'  `. .'   ` .'   ` /   \
# |    _  |   ' |    | |    | |    | |----' |   '
#  `.___| /     `.__/| /    |  `---| `.___, /
#                              \___/

if(BUILD_GRANGER)
  add_subdirectory(src/granger/src)
endif(BUILD_GRANGER)

#  _______ ___ ___ _______                              __ __
# |       |   |   |   |   | .----.-----.--------.-----.|__|  |.-----.----.
# |   -  _|   |   |       | |  __|  _  |        |  _  ||  |  ||  -__|   _|
# |_______|\_____/|__|_|__| |____|_____|__|__|__|   __||__|__||_____|__|
#                                               |__|
# TODO: DO NOT build unless building a QVM

add_subdirectory(src/tools)

#
# ,   . `
# |   | |
# |   | |
# `._/| /
#

add_subdirectory(src/ui)

#   ___.   ___  , _ , _     ___
# .'   `  /   ` |' `|' `. .'   `
# |    | |    | |   |   | |----'
#  `---| `.__/| /   '   / `.___,
#  \___/
#

add_subdirectory(src/game)

#   ___    ___.   ___  , _ , _     ___
# .'   ` .'   `  /   ` |' `|' `. .'   `
# |      |    | |    | |   |   | |----'
#  `._.'  `---| `.__/| /   '   / `.___,
#         \___/
#

add_subdirectory(src/cgame)

#            _____)                          ____  ___)
#          /                                (, /   /      /)
#         /   ___   __  _  __   _    _  __    /---/      (/_     _  ______
#        /     / ) / (_(_(_/ (_(_/__(/_/ (_) /   (__(_(_/_)  o  (__(_) // (_
#       (____ /               .-/         (_/
#                            (_/

# Author: Victor Roemer  wtfbbqhax, <victor@badsec.org>. 
#  Date: Sat Nov  5 06:49:08 2016 



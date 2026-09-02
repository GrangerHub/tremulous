#   M""""""""M                                       dP                             #
#   Mmmm  mmmM                                       88                             #
#   MMMM  MMMM 88d888b. .d8888b. 88d8b.d8b. dP    dP 88 .d8888b. dP    dP .d8888b.  #
#   MMMM  MMMM 88'  `88 88ooood8 88'`88'`88 88    88 88 88'  `88 88    88 Y8ooooo.  #
#   MMMM  MMMM 88       88.  ... 88  88  88 88.  .88 88 88.  .88 88.  .88       88  #
#   MMMM  MMMM dP       `88888P' dP  dP  dP `88888P' dP `88888P' `88888P' `88888P'  #
#   MMMMMMMMMM                                                                      #

cmake_minimum_required(VERSION 3.10...4.0)
project(Tremulous C CXX)

# ----------------------------------------------------------------------
# Protect the Makefile from being overwritten
# ----------------------------------------------------------------------
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR
        "\n"
        "=========================================================\n"
        "IN-SOURCE BUILD DETECTED AND REFUSED.\n"
        "\n"
        "CMake should NOT be run in the source directory because:\n"
        "  1. It would overwrite the hand-crafted Makefile.\n"
        "  2. It pollutes the source tree with build artifacts.\n"
        "\n"
        "Use a different build directory instead:\n"
        "  cmake -S . -B build-vs2022-x64 -G \"Visual Studio 17 2022\" -A x64\n"
        "  cmake -S . -B build-release -G \"Unix Makefiles\"\n"
        "=========================================================\n"
    )
endif()

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

# MSVC: generate debug info (.pdb) even in Release for easier debugging.
# The /Zi flag produces a separate .pdb; /DEBUG on the linker embeds the path.
if(MSVC)
    add_compile_options($<$<CONFIG:Release>:/Zi>)
    add_link_options($<$<CONFIG:Release>:/DEBUG>)
endif()

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

# ----------------------------------------------------------------------
# GCC/Clang-only compiler flags that mirror the Makefile.
# All are gated with generator expressions so MSVC is unaffected.
# ----------------------------------------------------------------------

# -fno-strict-aliasing: the Q3 engine type-puns aggressively (e.g. casting
# byte buffers to structs in qcommon/msg.cpp). Without this flag GCC's
# strict aliasing optimizer may reorder/reload from stale memory.
add_compile_options(
    $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-fno-strict-aliasing>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fno-strict-aliasing>
)

# -ffast-math (Release only): enables -fno-math-errno, -funsafe-math-
# optimizations, -ffinite-math-only, -fno-rounding-math, -fno-signaling-nans,
# -fcx-limited-range. The Makefile applies this to all Release targets.
# Not applied in Debug to preserve IEEE correctness for ASan/UBSan.
add_compile_options(
    $<$<AND:$<C_COMPILER_ID:GNU,Clang,AppleClang>,$<CONFIG:Release>>:-ffast-math>
    $<$<AND:$<CXX_COMPILER_ID:GNU,Clang,AppleClang>,$<CONFIG:Release>>:-ffast-math>
)

# Architecture-specific math optimization flags (GCC/Clang only).
# x86_64: enable SSE2 (default on x86_64 but explicit for parity).
# x86 (32-bit): use both x87 and SSE for floating point math.
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "amd64")
    add_compile_options(
        $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-msse2>
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-msse2>
    )
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "i386"
       OR CMAKE_SYSTEM_PROCESSOR STREQUAL "i686")
    add_compile_options(
        $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-msse2>
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-msse2>
        $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-mfpmath=387+sse>
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-mfpmath=387+sse>
    )
endif()

# -fvisibility=hidden for shared libraries (SHLIBCFLAGS in Makefile).
# Applied to SHARED targets only (renderers, game .so/.dll) via target
# properties in the per-module CMakeLists. Here we define it as a variable
# for reuse. See renderergl1/renderergl2/game/cgame/ui CMakeLists.txt.
set(SHLIB_CFLAGS
    "$<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-fvisibility=hidden>"
    "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fvisibility=hidden>"
    CACHE INTERNAL "Shared library visibility flags"
)

# -DUSE_ICON: enables icon support on Linux and MinGW.
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR MINGW)
    add_compile_options(-DUSE_ICON)
endif()

# -DUSE_LOCAL_HEADERS: use bundled headers from external/ instead of system.
add_compile_options(-DUSE_LOCAL_HEADERS)

# Threads: the Makefile links -lpthread on Linux. CMake's Threads::Threads
# is the portable equivalent.
find_package(Threads QUIET)

# Global include path — every target needs to resolve headers via their
# module-relative path (e.g. #include "qcommon/q_platform.h",
# #include "sys/sys_shared.h", #include "game/g_public.h"). Without this,
# MSVC builds fail with "Cannot open include file" because each
# add_subdirectory()'d CMakeLists only adds its OWN directory to the path.
include_directories(${CMAKE_SOURCE_DIR}/src)

# ----------------------------------------------------------------------
# Product Identity — Git Version Detection
# Derives version from git describe --tags, with fallback to "1.3.0 alpha".
# This is used as the default for GAME_PRODUCT_VERSION below.
# ----------------------------------------------------------------------
set(TREM_GIT_VERSION "")
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" describe --tags
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE _git_describe
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_VARIABLE _git_err
        RESULT_VARIABLE _git_rc
    )
    if(_git_rc EQUAL 0 AND _git_describe)
        # Sanitize '-' to '_' exactly like the Makefile does (sed -e 's/-/_/g').
        string(REPLACE "-" "_" TREM_GIT_VERSION "${_git_describe}")
    endif()
endif()
if(TREM_GIT_VERSION STREQUAL "")
    set(TREM_GIT_VERSION "1.3.0 alpha")
    message(STATUS "GAME_PRODUCT_VERSION: git describe unavailable, using fallback '${TREM_GIT_VERSION}'")
else()
    message(STATUS "GAME_PRODUCT_VERSION: derived from git -> '${TREM_GIT_VERSION}'")
endif()

# ----------------------------------------------------------------------
# === Build Targets ===
# Options for selecting which components to build.
# These are visible by default in cmake-gui.
# ----------------------------------------------------------------------
option(BUILD_CLIENT            "Build the tremulous client executable"                ON)
option(BUILD_SERVER            "Build the tremded dedicated server"                   ON)
option(BUILD_GAME_SO           "Build game/cgame/ui as native shared objects"         ON)
option(BUILD_GAME_QVM          "Build game/cgame/ui as QVM bytecode"                  ON)
option(BUILD_RENDERER_OPENGL2  "Build the OpenGL 2 renderer"                          ON)
option(BUILD_GRANGER           "Build the Granger build/premake tool"                 ON)

# ----------------------------------------------------------------------
# === Feature Toggles ===
# Core feature options that users commonly configure.
# These are visible by default in cmake-gui.
# ----------------------------------------------------------------------
option(USE_OPENAL         "Enable OpenAL audio backend"                           ON)
option(USE_CODEC_OPUS     "Enable Opus audio codec support"                       ON)
option(USE_CODEC_VORBIS   "Enable Vorbis audio codec support"                      ON)
option(USE_CURL           "Enable libcurl HTTP support"                           ON)
option(USE_RESTCLIENT     "Enable REST client (libcurl-based update checker)"     ON)
option(USE_VOIP           "Enable Voice-over-IP support"                          OFF)
option(USE_MUMBLE         "Enable Mumble positional audio link"                   OFF)

# ----------------------------------------------------------------------
# === Library Selection ===
# Options for choosing between vendored and system libraries.
# These are visible by default in cmake-gui.
# ----------------------------------------------------------------------
option(USE_INTERNAL_SDL3  "Use the vendored SDL3 from external/SDL3"             ON)
option(USE_INTERNAL_JPEG  "Use the vendored libjpeg from external/jpeg-8c"        ON)

# ----------------------------------------------------------------------
# === Product Identity ===
# Product name and version strings.
# These are visible by default in cmake-gui.
#
# OVERRIDE precedence (highest first):
#   1. config.yaml  GAME_PRODUCT_VERSION  (applied by build.bat via -C)
#   2. cmake -DGAME_PRODUCT_VERSION="..." on the command line
#   3. the git-derived default below (TREM_GIT_VERSION)
# A normal CACHE set never overwrites an already-cached value, so once an
# override is in the cache it sticks (until `build.bat clean`).
# ----------------------------------------------------------------------
set(GAME_PRODUCT_VERSION "${TREM_GIT_VERSION}" CACHE STRING "Product version string (becomes PRODUCT_VERSION define). Default = git describe --tags; override via config.yaml or -D.")
set(GAME_PRODUCT_NAME    "Tremulous"   CACHE STRING "Product name (becomes PRODUCT_NAME define; window title = name + version)")
message(STATUS "GAME_PRODUCT_VERSION effective value: '${GAME_PRODUCT_VERSION}'  (override via config.yaml)")

# Backward compatibility: support old TREM_* names if set
if(DEFINED TREM_PRODUCT_VERSION AND NOT DEFINED GAME_PRODUCT_VERSION_OVERRIDE)
    set(GAME_PRODUCT_VERSION "${TREM_PRODUCT_VERSION}")
    message(STATUS "GAME_PRODUCT_VERSION: using deprecated TREM_PRODUCT_VERSION='${TREM_PRODUCT_VERSION}'")
endif()
if(DEFINED TREM_PRODUCT_NAME AND NOT DEFINED GAME_PRODUCT_NAME_OVERRIDE)
    set(GAME_PRODUCT_NAME "${TREM_PRODUCT_NAME}")
    message(STATUS "GAME_PRODUCT_NAME: using deprecated TREM_PRODUCT_NAME='${TREM_PRODUCT_NAME}'")
endif()

# ----------------------------------------------------------------------
# === Advanced Options ===
# These options are rarely changed and are marked as Advanced.
# They only appear in cmake-gui when "Advanced" is checked.
# ----------------------------------------------------------------------
option(BUILD_GAME_QVM_11    "Build vanilla 1.1 format QVMs (legacy compatibility)" OFF)
option(BUILD_STANDALONE     "Build as standalone (no Tremulous branding)"          OFF)
option(BUILD_STATIC_LIBS    "Build all libraries as static (no shared DLLs)"        ON)
option(USE_RENDERER_DLOPEN  "Build renderers as dynamically loadable libraries"     ON)
option(USE_OPENAL_DLOPEN    "Dynamically load OpenAL at runtime (dlopen)"           ON)
option(USE_CURL_DLOPEN      "Dynamically load libcurl at runtime (dlopen)"         OFF)
option(USE_FREETYPE         "Enable FreeType font rendering"                       OFF)
option(USE_YACC             "Use YACC instead of LCC for parser generation"        OFF)
option(USE_INTERNAL_LIBS    "Use all vendored libraries from external/"            ON)
option(USE_INTERNAL_OGG     "Use vendored libogg from external/"                   ON)
option(USE_INTERNAL_VORBIS  "Use vendored libvorbis from external/"                ON)
option(USE_INTERNAL_OPUS    "Use vendored opus from external/"                     ON)
option(USE_INTERNAL_ZLIB    "Use vendored zlib from external/"                     ON)
option(USE_INTERNAL_LUA     "Use vendored Lua from external/"                      ON)
option(USE_INTERNAL_CURL    "Use vendored libcurl from external/"                  ON)
set(DEFAULT_BASEDIR "" CACHE STRING "Hardcoded default data directory (e.g. /usr/share/tremulous)")

# Mark all advanced options
mark_as_advanced(
    BUILD_GAME_QVM_11
    BUILD_STANDALONE
    BUILD_STATIC_LIBS
    USE_RENDERER_DLOPEN
    USE_OPENAL_DLOPEN
    USE_CURL_DLOPEN
    USE_FREETYPE
    USE_YACC
    USE_INTERNAL_LIBS
    USE_INTERNAL_OGG
    USE_INTERNAL_VORBIS
    USE_INTERNAL_OPUS
    USE_INTERNAL_ZLIB
    USE_INTERNAL_LUA
    USE_INTERNAL_CURL
    DEFAULT_BASEDIR
    USE_MUMBLE
    USE_NGHTTP2
    USE_OPENSSL_QUIC
    ENABLE_UNICODE
    CURL_LIBRARIES
)

# Apply defines based on toggles
if(USE_CURL)
    add_compile_options(-DUSE_CURL)
    if(USE_CURL_DLOPEN)
        add_compile_options(-DUSE_CURL_DLOPEN)
    endif()
endif()

if(USE_CODEC_VORBIS)
    add_compile_options(-DUSE_CODEC_VORBIS)
endif()

if(USE_FREETYPE)
    add_compile_options(-DBUILD_FREETYPE)
endif()

if(USE_MUMBLE)
    add_compile_options(-DUSE_MUMBLE)
endif()

if(USE_VOIP)
    add_compile_options(-DUSE_VOIP)
endif()

if(USE_CODEC_OPUS)
    add_compile_options(-DUSE_CODEC_OPUS)
endif()

if(USE_RESTCLIENT)
    add_compile_options(-DUSE_RESTCLIENT)
endif()

if(USE_OPENAL)
    add_compile_options(-DUSE_OPENAL)
    if(USE_OPENAL_DLOPEN)
        add_compile_options(-DUSE_OPENAL_DLOPEN)
    endif()
endif()

if(BUILD_STANDALONE)
    add_compile_options(-DSTANDALONE)
endif()

if(USE_RENDERER_DLOPEN)
    add_compile_options(-DUSE_RENDERER_DLOPEN)
endif()

if(USE_INTERNAL_JPEG)
    add_compile_options(-DUSE_INTERNAL_JPEG)
endif()

if(NOT DEFAULT_BASEDIR STREQUAL "")
    add_compile_options(-DDEFAULT_BASEDIR=\"${DEFAULT_BASEDIR}\")
endif()

include(${CMAKE_SOURCE_DIR}/cmake/build_dir)
include(${CMAKE_SOURCE_DIR}/cmake/debug_cflags)
include(${CMAKE_SOURCE_DIR}/cmake/suppress_external_options.cmake)

# builtins
# SDL3: build from source when the full source tree is present (fetched by
# CI/Docker or supplied by the user). The source itself is NOT vendored in
# git; without it we fall back to the bundled public headers in
# external/SDL3/include plus a system SDL3 found via pkg-config.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL3/CMakeLists.txt")
    add_subdirectory(external/SDL3 SDL3)
    suppress_sdl_options()
    suppress_libusb_options()

    # Record the SDL3 binary directory for SDL3.cmake fallback paths
    set(SDL3_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL3" CACHE PATH "SDL3 binary build directory")
else()
    message(STATUS "external/SDL3 source tree not found: using bundled headers + system SDL3")
    set(SDL3_PKG_FOUND FALSE)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL3_PKG sdl3)
    endif()
    if(NOT SDL3_PKG_FOUND)
        find_library(SDL3_SYSTEM_LIBRARY NAMES SDL3)
    endif()
    if(NOT SDL3_PKG_FOUND AND NOT SDL3_SYSTEM_LIBRARY)
        message(FATAL_ERROR
            "SDL3 not found: external/SDL3 has no source tree and no system SDL3 is available. "
            "Install SDL3 (with pkg-config metadata) or clone the SDL3 source (release-3.4.12) "
            "into external/SDL3 - see .github/workflows/build-packages.yml.")
    endif()
endif()

# OpenAL Soft is NOT vendored in git (LGPL). When a source tree is present
# (user-supplied or fetched by CI), build it as a static library linked
# directly into the client. Otherwise fall back to the classic OpenAL
# headers in external/AL and load the system OpenAL library at runtime
# (USE_OPENAL_DLOPEN, the default).
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/openal-soft/CMakeLists.txt")
    # LIBTYPE must be set here (not in suppression module) because it controls
    # how OpenAL builds and must be in cache before add_subdirectory processes.
    # All other OpenAL pre-sets (examples, tests, utils, install, backends) are
    # consolidated in cmake/suppress_external_options.cmake.
    set(LIBTYPE STATIC CACHE STRING "OpenAL library type" FORCE)
    add_subdirectory(external/openal-soft OpenAL)
    suppress_openal_options()
    set(OPENAL_VENDORED TRUE)
else()
    message(STATUS "external/openal-soft not found: using bundled external/AL headers + system OpenAL")
    set(OPENAL_VENDORED FALSE)
endif()

# libcurl: build from source when the full source tree is present (fetched
# by CI/Docker or supplied by the user). The source is NOT vendored in git;
# without it we use the bundled public headers in external/curl-src/include
# plus the system libcurl found via pkg-config.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/curl-src/CMakeLists.txt")
    # BUILD_STATIC_LIBS is an option() above (ON by default).  When enabled,
    # we also disable shared library building so all subprojects build static.
    if(BUILD_STATIC_LIBS)
        set(BUILD_SHARED_LIBS OFF)
    endif()
    # Note: Curl options are pre-set and suppressed in suppress_external_options.cmake
    # We keep HTTP/HTTPS/FTP/FTPS for map downloads, disable all others
    add_subdirectory(external/curl-src curl)
    suppress_curl_options()

    # Use the CMake target created by modern libcurl (creates ALIAS CURL::libcurl)
    set(CURL_LIBRARIES "libcurl" CACHE FILEPATH "Static libcurl library" FORCE)
else()
    message(STATUS "external/curl-src source tree not found: using bundled headers + system libcurl")
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(CURL_PKG libcurl)
    endif()
    if(NOT CURL_PKG_FOUND)
        message(FATAL_ERROR
            "libcurl not found: external/curl-src has no source tree and no system libcurl is "
            "available. Install libcurl (with pkg-config metadata) or clone the curl source "
            "into external/curl-src.")
    endif()
    set(CURL_LIBRARIES ${CURL_PKG_LIBRARIES} CACHE STRING "System libcurl libraries" FORCE)
endif()
set(CURL_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/external/curl-src/include" CACHE PATH "libcurl include directory" FORCE)

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

# =============================================================================
# QVM Packaging Target
# Creates a .pk3 (zip) file containing all QVMs in a vm/ subdirectory.
# Mirrors the Makefile's vms-gpp-$(VERSION).pk3 packaging.
#
# Override version components at configure time:
#   cmake -DQVM_VERSION_MAJOR=1.3.0 -DQVM_VERSION_TAG=alpha -DQVM_VERSION_INCREMENT=0.15 ..
#
# Build with:
#   cmake --build build/vs2022-x64 --config Release --target vms_pk3
# =============================================================================

set(QVM_VERSION_MAJOR "1.3.0" CACHE STRING "QVM major version (e.g., 1.3.0)")
set(QVM_VERSION_TAG "alpha" CACHE STRING "Release tag: alpha, beta, rc, release")
set(QVM_VERSION_INCREMENT "0.15" CACHE STRING "Incremental build number")

set(QVM_PK3_NAME "vms-v${QVM_VERSION_MAJOR}-${QVM_VERSION_TAG}.${QVM_VERSION_INCREMENT}.pk3")

# Output the .pk3 to the same directory as the built binaries.  For VS
# multi-config generators, CMAKE_RUNTIME_OUTPUT_DIRECTORY already points to
# the platform subdirectory, and the per-config folder (e.g. Release) is
# appended at build time via the $<CONFIG> generator expression.
if(CMAKE_CONFIGURATION_TYPES)
    set(QVM_PK3_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>")
else()
    set(QVM_PK3_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

add_custom_target(vms_pk3
    COMMAND ${CMAKE_COMMAND}
        -DBUILD_DIR=${CMAKE_BINARY_DIR}
        -DOUTPUT_DIR=${QVM_PK3_OUTPUT_DIR}
        -DPK3_NAME=${QVM_PK3_NAME}
        -P ${CMAKE_SOURCE_DIR}/cmake/PackageQVMs.cmake
    DEPENDS game.qvm cgame.qvm ui.qvm
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Packaging QVMs into ${QVM_PK3_NAME}"
    VERBATIM
)
message(STATUS "QVM packaging target 'vms_pk3' configured")
message(STATUS "  Output: ${QVM_PK3_OUTPUT_DIR}/${QVM_PK3_NAME}")

# =============================================================================
# Distribution target — mirrors `make dist` (Makefile:2926-2927)
# Creates a ZIP archive of the source tree via git archive.
# =============================================================================
add_custom_target(dist
    COMMAND ${GIT_EXECUTABLE} archive --format zip
            --output ${GAME_PRODUCT_NAME}-${GAME_PRODUCT_VERSION}.zip HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Creating source distribution ZIP"
    VERBATIM
)

# =============================================================================
# Install targets — mirrors `make install` (Makefile:2946-2970)
# Only active on Linux (matches Makefile behavior).
# =============================================================================
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    include(GNUInstallDirs)

    # Install binaries
    install(TARGETS tremulous tremded
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    # Install renderer shared libraries
    if(USE_RENDERER_DLOPEN)
        install(TARGETS renderergl1 renderergl2
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${GAME_PRODUCT_NAME}
        )
    endif()

    # Install game shared objects
    if(BUILD_GAME_SO)
        install(TARGETS cgame game ui
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${GAME_PRODUCT_NAME}/gpp
        )
    endif()

    # Install desktop entry, icon, metadata
    install(FILES misc/io.github.grangerhub.Tremulous.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
    )
    install(FILES misc/io.github.grangerhub.Tremulous.appdata.xml
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo
    )
    install(FILES misc/io.github.grangerhub.Tremulous.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps
    )
    install(FILES COPYING GPL CC
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/licenses/${GAME_PRODUCT_NAME}
    )
endif()

# =============================================================================
# Platform-specific notes (informational, not executable)
# =============================================================================
if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    message(STATUS "macOS build detected. The following frameworks may need linking:")
    message(STATUS "  Cocoa, OpenAL, Security, IOKit, OpenGL")
    message(STATUS "  Add via target_link_libraries(... -framework Cocoa)")
elseif(MINGW)
    message(STATUS "MinGW build detected. For fully static binaries add:")
    message(STATUS "  -static -static-libgcc -static-libstdc++")
endif()

message(STATUS "")
message(STATUS "=== Configuration Summary ===")
message(STATUS "  Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Architecture: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Version: ${GAME_PRODUCT_VERSION}")
message(STATUS "  Threads: ${CMAKE_THREAD_LIBS_INIT}")
message(STATUS "  Renderer DLOPEN: ${USE_RENDERER_DLOPEN}")
message(STATUS "  OpenAL: ${USE_OPENAL}")
message(STATUS "  Curl: ${USE_CURL}")
message(STATUS "  Opus: ${USE_CODEC_OPUS}")
message(STATUS "  Vorbis: ${USE_CODEC_VORBIS}")
message(STATUS "  RestClient: ${USE_RESTCLIENT}")
message(STATUS "  Standalone: ${BUILD_STANDALONE}")
message(STATUS "")


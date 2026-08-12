# =============================================================================
# Tremulous — Windows Cross-Compilation Dockerfile
# =============================================================================
# Cross-compiles Windows binaries (.exe / .dll / .qvm) from Linux using MinGW-w64.
#
# Output binaries (build/release-mingw32-x86_64/ or -x86/):
#
#   Applications:
#     tremulous.exe          — Game client
#     tremded.exe            — Dedicated server (SERVERBIN=tremded per Makefile)
#     granger.exe            — Lua mod management / build tool
#
#   Renderers (DLLs, loaded at runtime by tremulous.exe):
#     renderer_opengl1.dll   — Legacy OpenGL 1.x renderer
#     renderer_opengl2.dll   — Modern OpenGL 2+ renderer
#
#   SDL3 runtime:
#     SDL364.dll             — SDL3 for x86_64 (Tremulous naming convention)
#     SDL3.dll               — SDL3 for x86    (32-bit)
#
#   Game logic (native DLLs):
#     gpp/cgame.dll          — Client-side game logic
#     gpp/game.dll           — Server-side game logic
#     gpp/ui.dll             — User interface logic
#
#   Quake Virtual Machine (platform-independent bytecode):
#     gpp/vm/cgame.qvm       — Client game VM
#     gpp/vm/game.qvm        — Server game VM
#     gpp/vm/ui.qvm          — UI VM
#
#   Toolchain (for compiling QVMs natively on Windows):
#     tools/q3lcc.exe        — LCC compiler driver
#     tools/q3rcc.exe        — RCC compiler (called by q3lcc)
#     tools/q3cpp.exe        — C preprocessor (called by q3lcc)
#     tools/q3asm.exe        — Quake III assembler
#
# Usage:
#   # 64-bit build (default):
#   docker build -t tremulous:win64 .
#   docker run --rm tremulous:win64 cat /tremulous-windows-x86_64.zip > tremulous-win64.zip
#
#   # 32-bit build:
#   docker build --build-arg TARGET_ARCH=x86 -t tremulous:win32 .
#   docker run --rm tremulous:win32 cat /tremulous-windows-x86.zip > tremulous-win32.zip
#
#   # Or copy the entire output directory:
#   docker create --name tmp tremulous:win64
#   docker cp tmp:/output ./tremulous-win64
#   docker rm tmp
# =============================================================================

# syntax=docker/dockerfile:1
ARG TARGET_ARCH=x86_64

# -----------------------------------------------------------------------------
# Stage 1: Shared base — source copy, line-ending fix, asset setup
#          (shared between builder stages for better layer caching)
# -----------------------------------------------------------------------------
FROM debian:stable-slim AS build-base
WORKDIR /usr/src
ENV DEBIAN_FRONTEND=noninteractive \
    TERM=dumb

# Common build dependencies (combined for single layer + better caching).
# dos2unix added for CRLF→LF conversion (repo checked out on Windows).
# zip is needed by the Makefile's release target (it creates build/*.zip).
# cmake is needed to cross-compile SDL3 for MinGW (see Stage 1.5 below).
RUN apt-get update -y && apt-get install -y --no-install-recommends \
      make \
      build-essential \
      cmake \
      libgl1-mesa-dev \
      mingw-w64 \
      libsdl3-dev \
      libopenal-dev \
      libfreetype6-dev \
      zip \
      libcurl4-openssl-dev \
      curl \
      rsync \
      git \
      dos2unix \
      ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Copy entire source tree (includes assets/ submodule + external/ vendored libs)
COPY . /usr/src/

# ── Line-ending fix (CRITICAL FIX #1) ────────────────────────────────────────
# The repo is checked out on Windows with CRLF endings. The MinGW compiler and
# build tools choke on stray '\r' characters, so we convert to Unix LF.
#
# MUST include '*.md' — src/tools/lcc/src/dagcheck.md is processed by lburg to
# generate dagcheck.c. If it has CRLF, the generated C code has stray '\r'
# and compilation fails.
#
# Excluded: external/ (vendored, already LF), assets/ (submodule, handled
# separately below), .git/ (irrelevant to build).
RUN find /usr/src -type f \
      \( -name '*.c'      -o -name '*.h'    -o -name '*.cpp' -o -name '*.hpp' \
         -o -name '*.glsl' -o -name '*.y'   -o -name '*.l'   -o -name '*.in' \
         -o -name '*.asm'  -o -name '*.s'   -o -name '*.md' \) \
      ! -path '*/external/*' \
      ! -path '*/assets/*' \
      ! -path '*/.git/*' \
      -print0 | xargs -0 -r dos2unix --quiet 2>/dev/null || true

# ── Assets submodule (CRITICAL FIX #2) ───────────────────────────────────────
# The build system expects UI headers at build/assets/ui/ (referenced by
# src/ui/ui_shared.h → menudef.h). Copy from the submodule and fix line endings.
#
# MUST run dos2unix AFTER copying — the assets/ submodule is excluded from the
# main pass above, so copied files arrive with CRLF endings. Without this,
# the preprocessor throws syntax errors on menudef.h.
RUN mkdir -p /usr/src/build/assets/ui \
    && cp /usr/src/assets/ui/*.h /usr/src/build/assets/ui/ 2>/dev/null || true \
    && find /usr/src/build/assets -type f -print0 \
       | xargs -0 -r dos2unix --quiet 2>/dev/null || true

# ── Skip runtime data download ───────────────────────────────────────────────
# The Makefile's release target calls misc/download-paks.sh to fetch PK3 data
# files (maps, textures, sounds). These are runtime assets, not needed for
# compiling binaries. Replace with a no-op.
RUN printf '#!/bin/bash\nexit 0\n' > /usr/src/misc/download-paks.sh \
    && chmod +x /usr/src/misc/download-paks.sh

# -- OpenAL headers (FIX #4) --------------------------------------------------
# The Makefile's ALHDIR now points to external/openal-soft/include where
# the OpenAL headers actually live (AL/al.h, AL/alc.h). No symlink needed.

# -----------------------------------------------------------------------------
# Stage 1.5: SDL3 MinGW cross-compilation
#         (CRITICAL FIX #5 - produces the .a/.dll files the Makefile expects)
# -----------------------------------------------------------------------------
# The Makefile (lines 702-713) links against Tremulous-specific names:
#   win64: libSDL364main.a, libSDL364.dll.a, SDL364.dll
#   win32: libSDL3main.a,   libSDL3.dll.a,   SDL3.dll
#
# external/libs/win{32,64}/ currently contains only SDL2-named files left over
# from the SDL2 era.  The SDL3 source tree is vendored at external/SDL3/ but
# has never been compiled for MinGW.  This stage cross-compiles it and copies
# the outputs into the expected locations with the Tremulous naming convention.
#
# SDL3's CMake build always produces libSDL3.dll.a / libSDL3main.a / SDL3.dll
# regardless of architecture.  For x86_64 we rename to the "SDL364" convention
# expected by the Makefile; for x86 the standard names already match.
FROM build-base AS sdl3-builder
ARG TARGET_ARCH

RUN set -e; \
    if [ "${TARGET_ARCH}" = "x86_64" ]; then \
      MINGW_PREFIX="x86_64-w64-mingw32"; \
      LIBSDIR="/usr/src/external/libs/win64"; \
      SDL_PREFIX="SDL364"; \
    else \
      MINGW_PREFIX="i686-w64-mingw32"; \
      LIBSDIR="/usr/src/external/libs/win32"; \
      SDL_PREFIX="SDL3"; \
    fi; \
    echo "=== Cross-compiling SDL3 for ${MINGW_PREFIX} ===" ; \
    mkdir -p /tmp/sdl3-build && cd /tmp/sdl3-build ; \
    cmake /usr/src/external/SDL3 \
      -DCMAKE_SYSTEM_NAME=Windows \
      -DCMAKE_SYSTEM_PROCESSOR="${TARGET_ARCH}" \
      -DCMAKE_C_COMPILER="${MINGW_PREFIX}-gcc" \
      -DCMAKE_CXX_COMPILER="${MINGW_PREFIX}-g++" \
      -DCMAKE_RC_COMPILER="${MINGW_PREFIX}-windres" \
      -DCMAKE_INSTALL_PREFIX=/tmp/sdl3-install \
      -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
      -DCMAKE_C_FLAGS="-I/usr/${MINGW_PREFIX}/include" \
      -DCMAKE_CXX_FLAGS="-I/usr/${MINGW_PREFIX}/include" \
      -DSDL_SHARED=ON \
      -DSDL_STATIC=OFF \
      -DSDL_TEST=OFF \
      -DSDL_VULKAN=OFF \
      -DCMAKE_BUILD_TYPE=Release ; \
    cmake --build . -j"$(nproc)" ; \
    echo "=== Installing SDL3 to /tmp/sdl3-install ===" ; \
    cmake --install . ; \
    \
    echo "=== Checking installed SDL3 files ===" ; \
    find /tmp/sdl3-install -type f -name '*.a' -o -name '*.dll' -o -name '*.so' ; \
    \
    echo "=== Installing SDL3 libs to ${LIBSDIR} ===" ; \
    mkdir -p "${LIBSDIR}" ; \
    if [ -f /tmp/sdl3-install/bin/SDL3.dll ]; then \
      cp /tmp/sdl3-install/bin/SDL3.dll "${LIBSDIR}/${SDL_PREFIX}.dll" ; \
    else echo "ERROR: SDL3.dll not found in /tmp/sdl3-install/bin/" ; ls -la /tmp/sdl3-install/bin/ ; fi ; \
    if [ -f /tmp/sdl3-install/lib/libSDL3.dll.a ]; then \
      cp /tmp/sdl3-install/lib/libSDL3.dll.a "${LIBSDIR}/lib${SDL_PREFIX}.dll.a" ; \
    else echo "ERROR: libSDL3.dll.a not found in /tmp/sdl3-install/lib/" ; ls -la /tmp/sdl3-install/lib/ ; fi ; \
    # SDL3 main library - find it under the arch-specific subdir or flat
    MAIN_A=$(find /tmp/sdl3-install -name 'libSDL3main.a' | head -1) ; \
    if [ -n "${MAIN_A}" ]; then \
      echo "Found SDL3main.a at: ${MAIN_A}" ; \
      cp "${MAIN_A}" "${LIBSDIR}/lib${SDL_PREFIX}main.a" ; \
    else echo "WARNING: libSDL3main.a not found, searching for SDL3main*.a..." ; \
         find /tmp/sdl3-install -name '*SDL3main*' ; fi ; \
    echo "=== Final contents of ${LIBSDIR} ===" ; \
    ls -la "${LIBSDIR}" ; \
    echo "=== SDL3 cross-compilation complete ==="

# -----------------------------------------------------------------------------
# Stage 2: Windows builder — compile all binaries + Windows-native tools
# -----------------------------------------------------------------------------
FROM sdl3-builder AS builder
ARG TARGET_ARCH

# ═─ Main build ─═════════════════════════════════════════════════════════════
# Build flags (matching .github/workflows/build-packages.yml):
#   PLATFORM=mingw32       — Target MinGW-w64 (auto-selects cross-compiler)
#   ARCH=x86_64 | x86      — 64-bit or 32-bit
#   USE_RESTCLIENT=1       — Enable REST client integration
#   USE_INTERNAL_LUA=1     — Use vendored Lua 5.3.3 from external/lua-5.3.3/
#   USE_CURL_DLOPEN=0      — Statically link libcurl (required for MinGW)
#
# The Makefile auto-detects the MinGW cross-compiler based on ARCH:
#   x86_64 → x86_64-w64-mingw32-gcc / x86_64-w64-mingw32-g++
#   x86    → i686-w64-mingw32-gcc   / i686-w64-mingw32-g++
#
# Vendored Windows libraries in external/libs/win{32,64}/ are used automatically
# (USE_INTERNAL_LIBS=1 and USE_LOCAL_HEADERS=1 are defaults) — no SDL3 download
# needed.
RUN USE_RESTCLIENT=1 USE_INTERNAL_LUA=1 USE_CURL_DLOPEN=0 \
    BUILD_GAME_QVM=0 BUILD_GAME_QVM_11=0 \
    make PLATFORM=mingw32 ARCH="${TARGET_ARCH}" -j"$(nproc)"

# ═─ Build Windows-native toolchain (CRITICAL FIX #3) ─═══════════════════════
# q3lcc.exe, q3asm.exe, q3rcc.exe, q3cpp.exe
#
# The main build above compiles the LCC/ASM tools as LINUX binaries (host tools)
# because they're needed during the build to generate QVM bytecode. This step
# REBUILDS those same tools as Windows .exe files so end-users can compile QVMs
# natively on Windows without needing Docker.
#
# Strategy:
#   1. Delete the Linux-compiled tool .o files (force recompilation with MinGW)
#   2. Use `make -o dagcheck.c` to tell make NOT to regenerate dagcheck.c
#      (regeneration requires running lburg, which would be a Windows binary
#       that can't execute on Linux). dagcheck.c already exists from the main
#       build, so we reuse it as-is.
#   3. Override TOOLS_CC and TOOLS_BINEXT to produce Windows binaries.
#   4. Build only the tool targets (disable all other BUILD_* flags).
#
# CRITICAL: Must pass B=... on the command line because the Makefile only sets B
# inside its recursive release/debug sub-make calls (line 1147: make targets B=$(BR)).
# Without this, $(Q3LCC) resolves to /tools/q3lcc.exe (empty prefix) and make
# reports "No rule to make target".
RUN set -e; \
    BUILD_DIR="build/release-mingw32-${TARGET_ARCH}"; \
    if [ "${TARGET_ARCH}" = "x86_64" ]; then \
      MINGW_CC="x86_64-w64-mingw32-gcc"; \
    else \
      MINGW_CC="i686-w64-mingw32-gcc"; \
    fi; \
    DAGCHECK_C="${BUILD_DIR}/tools/rcc/dagcheck.c"; \
    echo "=== Building Windows-native tools (${MINGW_CC}) ===" ; \
    find "${BUILD_DIR}/tools" -name '*.o' -delete 2>/dev/null || true ; \
    make PLATFORM=mingw32 ARCH="${TARGET_ARCH}" \
         B="${BUILD_DIR}" \
         USE_RESTCLIENT=1 USE_INTERNAL_LUA=1 USE_CURL_DLOPEN=0 \
         TOOLS_CC="${MINGW_CC}" \
         TOOLS_BINEXT=.exe \
         TOOLS_LDFLAGS="-static -static-libgcc" \
         BUILD_CLIENT=0 BUILD_SERVER=0 BUILD_GAME_SO=0 BUILD_GAME_QVM=0 BUILD_GAME_QVM_11=0 BUILD_GRANGER=0 \
         -o "${DAGCHECK_C}" \
         "${BUILD_DIR}/tools/q3lcc.exe" \
         "${BUILD_DIR}/tools/q3rcc.exe" \
         "${BUILD_DIR}/tools/q3cpp.exe" \
         "${BUILD_DIR}/tools/q3asm.exe" \
         -j"$(nproc)" \
    && echo "=== Windows tools built successfully ===" \
    || echo "WARNING: Windows tools build failed - q3lcc.exe/q3asm.exe not included"

# ── Verification: list all produced binaries ─────────────────────────────────
RUN echo "=== Build output ===" \
    && find "build/release-mingw32-${TARGET_ARCH}" -type f \
       \( -name '*.exe' -o -name '*.dll' -o -name '*.qvm' \) \
    | sort \
    && echo "===================="

# -----------------------------------------------------------------------------
# Stage 3: Package — minimal image with just the binaries + zip
# -----------------------------------------------------------------------------
FROM debian:stable-slim AS package
ARG TARGET_ARCH
WORKDIR /output

RUN apt-get update -y && apt-get install -y --no-install-recommends zip \
    && rm -rf /var/lib/apt/lists/*

# Copy only the release output directory (not debug builds or .o files)
COPY --from=builder "/usr/src/build/release-mingw32-${TARGET_ARCH}/" /output/

# Show what's included
RUN echo "=== Package contents ===" \
    && find /output -type f | sort \
    && echo "========================"

# Create a distributable zip
RUN cd /output \
    && zip -rq "/tremulous-windows-${TARGET_ARCH}.zip" .

CMD ["sh", "-c", "echo 'Extract: docker run --rm tremulous:win64 cat /tremulous-windows-x86_64.zip > output.zip'"]

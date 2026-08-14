#
##  .o88b. db      d888888b d88888b d8b   db d888888b
## d8P  Y8 88        `88'   88'     888o  88 `~~88~~'
## 8P      88         88    88ooooo 88V8o 88    88
## 8b      88         88    88~~~~~ 88 V8o88    88
## Y8b  d8 88booo.   .88.   88.     88  V888    88
##  `Y88P' Y88888P Y888888P Y88888P VP   V8P    YP
#

# libcurl is now built from source via add_subdirectory in top-level CMakeLists.txt
# CURL_LIBRARIES and CURL_INCLUDE_DIRS are set in root CMakeLists.txt for static linking
# Use those variables if they exist, otherwise try to find curl targets
if(NOT CURL_LIBRARIES OR NOT CURL_INCLUDE_DIRS)
    if(TARGET libcurl::libcurl)
        set(CURL_LIBRARIES libcurl::libcurl)
        # Get the include directory from the target
        get_target_property(CURL_INCLUDE_DIRS libcurl::libcurl INTERFACE_INCLUDE_DIRECTORIES)
    elseif(TARGET CURL::libcurl)
        set(CURL_LIBRARIES CURL::libcurl)
        get_target_property(CURL_INCLUDE_DIRS CURL::libcurl INTERFACE_INCLUDE_DIRECTORIES)
    else()
        # Fallback to find_package for system installations
        find_package(CURL)
    endif()
endif()

find_package(OpenGL)
include(${CMAKE_SOURCE_DIR}/cmake/SDL3.cmake)

# OpenAL Soft: when the source tree is present it is built as a static
# subdirectory and linked directly. When absent (it is not vendored in git),
# fall back to the bundled external/AL headers and load the system OpenAL
# library at runtime (USE_OPENAL_DLOPEN).
if(OPENAL_VENDORED)
    set(OPENAL_LIBRARY OpenAL::OpenAL)
    set(OPENAL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/openal-soft/include")
else()
    set(OPENAL_LIBRARY "")
    set(OPENAL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/AL")
endif()

# CURL_STATICLIB is only correct when libcurl is built as a static library
# from the vendored source tree (system libcurl fallback is shared).
if(TARGET libcurl)
    add_definitions(-DCURL_STATICLIB)
endif()

add_definitions(
    -DUSE_LOCAL_HEADERS
    -DUSE_RESTCLIENT
    -DUSE_CODEC_OPUS
    -DUSE_OPENAL
    -DUSE_INTERNAL_JPEG
    -DUSE_RENDERER_DLOPEN
    -DNDEBUG
    -DPRODUCT_VERSION="${GAME_PRODUCT_VERSION}"
    -DPRODUCT_NAME="${GAME_PRODUCT_NAME}"
    ${SDL3_DEFINES}
    )

set(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/external)

# Platform-conditional sys layer sources.
# sys_unix.cpp / con_tty.cpp use POSIX headers (unistd.h, dirent.h,
# sys/time.h) that MSVC does not provide. Use the win32 equivalents on
# Windows instead.
if(WIN32)
    set(SYS_PLATFORM_SOURCES
        ${PARENT_DIR}/sys/con_win32.cpp
        ${PARENT_DIR}/sys/sys_win32.cpp
        ${PARENT_DIR}/sys/sys_win32_default_homepath.cpp
        )
else()
    set(SYS_PLATFORM_SOURCES
        ${PARENT_DIR}/sys/con_tty.cpp
        ${PARENT_DIR}/sys/sys_unix.cpp
        )
endif()

if(APPLE)
set(APPLE_SOURCES ${PARENT_DIR}/sys/sys_osx.mm)
endif(APPLE)

add_executable(
    tremulous
    #
    # snapvector.c uses GCC inline asm (__asm__ volatile AT&T syntax).
    # Only include it when not building with MSVC (idx64/id386 are gated
    # to __GNUC__ in q_platform.h, so MSVC uses the portable C fallback).
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:${PARENT_DIR}/asm/snapvector.c>
    #
    cl_avi.cpp
    cl_cgame.cpp
    cl_cin.cpp
    cl_console.cpp
    cl_curl.cpp
    cl_input.cpp
    cl_keys.cpp
    cl_main.cpp
    cl_net_chan.cpp
    cl_parse.cpp
    cl_rest.cpp
    cl_scrn.cpp
    cl_ui.cpp
    cl_updates.cpp
    libmumblelink.cpp
    qal.cpp
    snd_adpcm.cpp
    snd_codec.cpp
    snd_codec_ogg.cpp
    snd_codec_opus.cpp
    snd_codec_wav.cpp
    snd_dma.cpp
    snd_main.cpp
    snd_mem.cpp
    snd_mix.cpp
    snd_openal.cpp
    snd_wavelet.cpp
    #
    ${PARENT_DIR}/qcommon/cm_load.cpp
    ${PARENT_DIR}/qcommon/cm_patch.cpp
    ${PARENT_DIR}/qcommon/cm_polylib.cpp
    ${PARENT_DIR}/qcommon/cm_test.cpp
    ${PARENT_DIR}/qcommon/cm_trace.cpp
    ${PARENT_DIR}/qcommon/cmd.cpp
    ${PARENT_DIR}/qcommon/common.cpp
    ${PARENT_DIR}/qcommon/crypto.cpp
    ${PARENT_DIR}/qcommon/cvar.cpp
    ${PARENT_DIR}/qcommon/cvar.h
    ${PARENT_DIR}/qcommon/files.cpp
    ${PARENT_DIR}/qcommon/files.h
    ${PARENT_DIR}/qcommon/huffman.cpp
    ${PARENT_DIR}/qcommon/huffman.h
    ${PARENT_DIR}/qcommon/ioapi.cpp
    ${PARENT_DIR}/qcommon/md4.cpp
    ${PARENT_DIR}/qcommon/md5.cpp
    ${PARENT_DIR}/qcommon/msg.cpp
    ${PARENT_DIR}/qcommon/msg.h
    ${PARENT_DIR}/qcommon/net_chan.cpp
    ${PARENT_DIR}/qcommon/net_ip.cpp
    ${PARENT_DIR}/qcommon/net.h
    ${PARENT_DIR}/qcommon/parse.cpp
    ${PARENT_DIR}/qcommon/puff.cpp
    ${PARENT_DIR}/qcommon/q_shared.c
    ${PARENT_DIR}/qcommon/q3_lauxlib.cpp
    ${PARENT_DIR}/qcommon/q_math.c
    ${PARENT_DIR}/qcommon/unzip.cpp
    ${PARENT_DIR}/qcommon/vm.cpp
    ${PARENT_DIR}/qcommon/vm_interpreted.cpp
    # vm_x86.cpp uses 32-bit inline assembly not supported on MSVC x64
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:${PARENT_DIR}/qcommon/vm_x86.cpp>
    #
    ${PARENT_DIR}/sdl/sdl_input.cpp
    ${PARENT_DIR}/sdl/sdl_snd.cpp
    #
    ${PARENT_DIR}/server/sv_ccmds.cpp
    ${PARENT_DIR}/server/sv_client.cpp
    ${PARENT_DIR}/server/sv_game.cpp
    ${PARENT_DIR}/server/sv_init.cpp
    ${PARENT_DIR}/server/sv_main.cpp
    ${PARENT_DIR}/server/sv_net_chan.cpp
    ${PARENT_DIR}/server/sv_snapshot.cpp
    ${PARENT_DIR}/server/sv_world.cpp
    #
    ${PARENT_DIR}/sys/con_log.cpp
    ${PARENT_DIR}/sys/sys_main.cpp
    ${PARENT_DIR}/sys/sys_shared.h
    ${SYS_PLATFORM_SOURCES}
    ${APPLE_SOURCES}

    ${EXTERNAL_DIR}/semver/src/lib/semantic_version_v1.cpp
    ${EXTERNAL_DIR}/semver/src/lib/semantic_version_v2.cpp
    )

if(APPLE)
 # FIXME Prefixed with "lua" to prevent cmake from doing "-l-framework Cocoa"
 set(FRAMEWORKS "-framework Cocoa -framework Security -framework OpenAL -framework IOKit")
else(APPLE)
 if(WIN32)
  # Windows-specific libraries for networking and multimedia
  # Windows libraries for networking, multimedia, and static libcurl dependencies
  # wldap32: LDAP (libcurl LDAP support)
  # crypt32: Certificate API (libcurl TLS)
  # iphlpapi: IP Helper API (libcurl network interface enumeration)
  # bcrypt: CNG random (libcurl entropy)
  # secur32: SSPI/SChannel (libcurl Windows TLS)
  # normaliz: IDN normalization (libcurl international domain names)
  set(SYSLIBS ws2_32 winmm wldap32 crypt32 iphlpapi bcrypt secur32 normaliz)
 elseif(UNIX)
  set(SYSLIBS dl rt)
 endif()
endif(APPLE)

# MSVC x64 doesn't support inline assembly, so vm_x86.cpp is excluded.
# Define NO_VM_COMPILED to fall back to the bytecode interpreter.
if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    target_compile_definitions(tremulous PRIVATE NO_VM_COMPILED)
endif()

if(NOT USE_RENDERER_DLOPEN)
  if(USE_OPENGL1)
  set(RENDERER_LIBRARY renderergl1)
  endif(USE_OPENGL1)
  set(RENDERER_LIBRARY renderergl2)
endif(NOT USE_RENDERER_DLOPEN)

target_link_libraries(
    tremulous
    #
    lua
    nettle
    zlib
    ogg
    vorbis
    opus
    opusfile
    #
    script_api
    restclient
    #
    ${FRAMEWORKS}
    ${CURL_LIBRARIES}
    ${SDL3_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${OPENAL_LIBRARY}
    ${SYSLIBS}
    )

include_directories(
    ${PARENT_DIR}
    ${PARENT_DIR}/qcommon
    ${EXTERNAL_DIR}/zlib
    ${EXTERNAL_DIR}/restclient
    ${EXTERNAL_DIR}/rapidjson
    ${EXTERNAL_DIR}/SDL3/include/SDL3  # For flat #include "SDL.h"
    ${EXTERNAL_DIR}/SDL3/include/build_config  # For SDL_build_config.h
    ${SDL3_INCLUDE_DIRS}  # SDL3 include directories from cmake/SDL3.cmake
    ${EXTERNAL_DIR}/jpeg-8c
    ${EXTERNAL_DIR}/libogg-1.3.2/include
    ${EXTERNAL_DIR}/libvorbis-1.3.5/include
    ${EXTERNAL_DIR}/lua-5.3.3/include
    ${EXTERNAL_DIR}/sol
    ${CURL_INCLUDE_DIRS}  # libcurl include directory
    ${EXTERNAL_DIR}/nettle-3.3
    ${EXTERNAL_DIR}/opus-1.1.4/include
    ${EXTERNAL_DIR}/opusfile-0.8/include
    ${EXTERNAL_DIR}/restclient
    ${EXTERNAL_DIR}/semver/src/include
    ${SDL3_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIR}
    )

# TODO: Turn this into a macro
if (USE_INTERNAL_SDL3)
    # SDL3 on macOS: copy the shared library to runtime directory
    if(TARGET SDL3::SDL3-shared)
        add_custom_command(
            TARGET tremulous
            POST_BUILD COMMAND ${CMAKE_COMMAND}
            ARGS -E copy $<TARGET_FILE:SDL3::SDL3-shared> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
            )
    elseif(EXISTS ${CMAKE_SOURCE_DIR}/external/SDL2/libs/Darwin/libSDL2-2.0.0.dylib)
        # Fallback to old SDL2 location
        add_custom_command(
            TARGET tremulous
            POST_BUILD COMMAND ${CMAKE_COMMAND}
            ARGS -E copy ${CMAKE_SOURCE_DIR}/external/SDL2/libs/Darwin/libSDL2-2.0.0.dylib ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libSDL2-2.0.0.dylib
            )
    endif()
endif(USE_INTERNAL_SDL3)

if (USE_RENDERER_DLOPEN)
    # Renderer DLLs are already output to the runtime directory by their
    # CMake targets (renderergl1.dll, renderergl2.dll). No rename needed.
endif(USE_RENDERER_DLOPEN)

add_dependencies(
    tremulous
    ui
    game
    cgame
    renderergl1
    renderergl2
    )

# --- Visual Studio debugging configuration ---
# Set the working directory to the game data install so the engine can find
# pk3 files, DLLs, and the gpp/ game directory.  Override at configure time:
#   cmake -DTREMULOUS_DEBUG_DIR="C:/my/tremulous/install" ...
set(TREMULOUS_DEBUG_DIR "F:/Program Files/Tremulous" CACHE PATH
    "Working directory for debugging tremulous from Visual Studio")

set_target_properties(tremulous PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${TREMULOUS_DEBUG_DIR}"
    VS_DEBUGGER_COMMAND_ARGUMENTS "+set fs_basepath \"${TREMULOUS_DEBUG_DIR}\" +set r_fullscreen 0 +set vm_game 0 +set vm_cgame 0 +set vm_ui 0"
)

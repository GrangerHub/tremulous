# suppress_external_options.cmake
#
# This module suppresses and pre-sets external library options that leak into
# the parent CMake project via add_subdirectory(), reducing cmake-gui clutter.
#
# Usage: Include this file BEFORE any add_subdirectory(external/...) calls,
# then call the respective suppression macro AFTER each add_subdirectory().
#
# Example:
#   include(${CMAKE_SOURCE_DIR}/cmake/suppress_external_options.cmake)
#   add_subdirectory(external/openal-soft)
#   suppress_openal_options()

# ============================================================================
# PRE-SET FORCE BLOCKS
# These set values BEFORE external libraries process their options,
# preventing default values from being added to cache.
# ============================================================================

# --------------------------------------------------------------------------
# OpenAL Soft Pre-sets
# --------------------------------------------------------------------------
# Disable examples, tests, utils, install options
# (Moved from CMakeLists.txt Phase 4.1 — all OpenAL pre-sets consolidated here)
set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALSOFT_TESTS    OFF CACHE BOOL "" FORCE)
set(ALSOFT_UTILS    OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL  OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_TESTS OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_UTILS OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_CONFIG OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_HRTF_DATA OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_AMBDEC_PRESETS OFF CACHE BOOL "" FORCE)
set(ALSOFT_UPDATE_BUILD_VERSION OFF CACHE BOOL "" FORCE)
set(ALSOFT_DLOPEN OFF CACHE BOOL "" FORCE)
set(ALSOFT_WERROR OFF CACHE BOOL "" FORCE)

# Disable backends irrelevant to Windows
set(ALSOFT_REQUIRE_JACK OFF CACHE BOOL "" FORCE)
set(ALSOFT_REQUIRE_PULSEAUDIO OFF CACHE BOOL "" FORCE)
set(ALSOFT_REQUIRE_ALSA OFF CACHE BOOL "" FORCE)
set(ALSOFT_REQUIRE_OSS OFF CACHE BOOL "" FORCE)
set(ALSOFT_REQUIRE_SOLARIS OFF CACHE BOOL "" FORCE)
set(ALSOFT_REQUIRE_SNDIO OFF CACHE BOOL "" FORCE)
set(ALSOFT_REQUIRE_COREAUDIO OFF CACHE BOOL "" FORCE)
set(ALSOFT_REQUIRE_OBOE OFF CACHE BOOL "" FORCE)
set(ALSOFT_REQUIRE_OPENSL OFF CACHE BOOL "" FORCE)
set(ALSOFT_REQUIRE_PORTAUDIO OFF CACHE BOOL "" FORCE)
set(ALSOFT_REQUIRE_PIPEWIRE OFF CACHE BOOL "" FORCE)
set(ALSOFT_REQUIRE_SDL2 OFF CACHE BOOL "" FORCE)
set(ALSOFT_REQUIRE_SDL3 OFF CACHE BOOL "" FORCE)

# --------------------------------------------------------------------------
# Curl Pre-sets
# --------------------------------------------------------------------------
# Individual protocol disables (NOT HTTP_ONLY - must preserve FTP/FTPS for map downloads)
set(CURL_DISABLE_DICT ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_TELNET ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_TFTP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_POP3 ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_SMTP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_IMAP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_GOPHER ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_RTSP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_SMB ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_MQTT ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_LDAP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_LDAPS ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_DOH ON CACHE BOOL "" FORCE)

# Disable LDAP for Windows (USE_WIN32_LDAP)
set(CURL_USE_LDAP OFF CACHE BOOL "" FORCE)
set(CURL_USE_LDAPS OFF CACHE BOOL "" FORCE)

# Disable curl features we don't need
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)           # We don't need curl.exe
set(BUILD_LIBCURL_DOCS OFF CACHE BOOL "" FORCE)       # Don't build man pages
set(ENABLE_ARES OFF CACHE BOOL "" FORCE)              # c-ares DNS resolver (not needed)
set(ENABLE_CURLDEBUG OFF CACHE BOOL "" FORCE)         # TrackMemory debugging
set(ENABLE_CURL_MANUAL OFF CACHE BOOL "" FORCE)       # curl manual page
set(ENABLE_DEBUG OFF CACHE BOOL "" FORCE)             # curl debug features
set(ENABLE_WEBSOCKETS OFF CACHE BOOL "" FORCE)        # Experimental websockets
set(FORCE_STATIC_VCRT OFF CACHE BOOL "" FORCE)        # /MT for static VC runtime (OpenAL)
set(PICKY_COMPILER OFF CACHE BOOL "" FORCE)           # Extra compiler warnings
set(USE_MSH3 OFF CACHE BOOL "" FORCE)                 # msquic for HTTP/3
set(USE_NGTCP2 OFF CACHE BOOL "" FORCE)               # ngtcp2/nghttp3 for HTTP/3
set(USE_QUICHE OFF CACHE BOOL "" FORCE)               # quiche for HTTP/3
set(USE_WIN32_IDN OFF CACHE BOOL "" FORCE)            # WinIDN for IDN support

# Curl build options (moved from CMakeLists.txt Phase 2.2)
set(CURL_DISABLE_TESTS ON CACHE BOOL "" FORCE)         # Disable curl tests
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)             # Disable CMake testing
set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)          # Disable libpsl
set(CURL_ZLIB OFF CACHE BOOL "" FORCE)                # Disable zlib in curl (we use our own)
set(CURL_BROTLI OFF CACHE BOOL "" FORCE)              # Disable brotli
set(CURL_ZSTD OFF CACHE BOOL "" FORCE)                # Disable zstd
set(CURL_NGHTTP2 OFF CACHE BOOL "" FORCE)             # Disable nghttp2 in curl (we use our own)
set(CURL_USE_LIBIDN2 OFF CACHE BOOL "" FORCE)         # Disable libidn2 in curl

# --------------------------------------------------------------------------
# SDL3 Pre-sets
# --------------------------------------------------------------------------
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL_CMAKE OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL_MAN OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL_PKG_CONFIG OFF CACHE BOOL "" FORCE)

# ============================================================================
# SUPPRESSION MACROS
# These mark external library options as advanced (hidden from default view)
# AFTER the add_subdirectory() call.
# These use file(GLOB) to find ALL matching cache variables.
# ============================================================================

# --------------------------------------------------------------------------
# Suppress OpenAL Soft Options
# --------------------------------------------------------------------------
macro(suppress_openal_options)
    # Mark all ALSOFT_* variables as advanced
    get_cmake_property(_vars CACHE_VARIABLES)
    foreach(_var ${_vars})
        if(_var MATCHES "^ALSOFT_")
            mark_as_advanced(FORCE ${_var})
        endif()
    endforeach()

    # Also suppress OpenAL-specific variables that don't have ALSOFT_ prefix
    mark_as_advanced(FORCE
        FORCE_STATIC_VCRT
        LIBTYPE
    )
endmacro()

# --------------------------------------------------------------------------
# Suppress Curl Options
# --------------------------------------------------------------------------
macro(suppress_curl_options)
    # Mark all CURL_* variables as advanced
    get_cmake_property(_vars CACHE_VARIABLES)
    foreach(_var ${_vars})
        if(_var MATCHES "^CURL_")
            mark_as_advanced(FORCE ${_var})
        endif()
    endforeach()

    # Also suppress curl-specific variables that don't have CURL_ prefix
    mark_as_advanced(FORCE
        BUILD_CURL_EXE
        BUILD_LIBCURL_DOCS
        BUILD_STATIC_CURL
        ENABLE_ARES
        ENABLE_CURLDEBUG
        ENABLE_CURL_MANUAL
        ENABLE_DEBUG
        ENABLE_THREADED_RESOLVER
        ENABLE_UNIX_SOCKETS
        ENABLE_WEBSOCKETS
        LIBCURL_OUTPUT_NAME
        PICKY_COMPILER
        USE_LIBIDN2
        USE_MSH3
        USE_NGTCP2
        USE_QUICHE
        USE_WIN32_IDN
    )

    # Suppress any AudioUnit or PulseAudio paths (not relevant on Windows)
    mark_as_advanced(FORCE
        AUDIOUNIT_INCLUDE_DIR
        AUDIOUNIT_LIBRARY
        PULSEAUDIO_INCLUDE_DIR
        PULSEAUDIO_LIBRARY
    )
endmacro()

# --------------------------------------------------------------------------
# Suppress SDL3 Options
# --------------------------------------------------------------------------
macro(suppress_sdl_options)
    # Mark all SDL_* variables as advanced
    get_cmake_property(_vars CACHE_VARIABLES)
    foreach(_var ${_vars})
        if(_var MATCHES "^SDL_")
            mark_as_advanced(FORCE ${_var})
        endif()
    endforeach()
endmacro()

# --------------------------------------------------------------------------
# Suppress LibUSB Options
# --------------------------------------------------------------------------
macro(suppress_libusb_options)
    # Mark all LibUSB and LIBUSB variables as advanced (case-insensitive)
    get_cmake_property(_vars CACHE_VARIABLES)
    foreach(_var ${_vars})
        if(_var MATCHES "[Ll][Ii][Bb][Uu][Ss][Bb]")
            mark_as_advanced(FORCE ${_var})
        endif()
    endforeach()
endmacro()

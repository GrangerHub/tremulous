# SDL3 integration - built from source via add_subdirectory in top-level CMakeLists.txt
# SDL3 provides CMake targets automatically when built as subdirectory.
# When the full source tree is absent (it is not vendored in git), the
# top-level CMakeLists.txt locates a system SDL3 via pkg-config and sets
# SDL3_PKG_FOUND / SDL3_SYSTEM_LIBRARY, which are consumed here.

if(USE_INTERNAL_SDL3)
    set(SDL3_DEFINES "-DUSE_LOCAL_HEADERS=1")
    if(TARGET SDL3::SDL3-shared)
        # SDL3 built from source, use its CMake target
        set(SDL3_LIBRARIES SDL3::SDL3-shared)
        get_target_property(SDL3_INCLUDE_DIRS SDL3::SDL3-shared INTERFACE_INCLUDE_DIRECTORIES)
    elseif(TARGET SDL3::SDL3)
        set(SDL3_LIBRARIES SDL3::SDL3)
        get_target_property(SDL3_INCLUDE_DIRS SDL3::SDL3 INTERFACE_INCLUDE_DIRECTORIES)
    elseif(SDL3_PKG_FOUND)
        # System SDL3 located via pkg-config in the top-level CMakeLists.txt.
        # Prefer the bundled headers (they match the vendored release) but
        # keep the pkg-config paths so SDL_revision/build config stay correct.
        set(SDL3_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/external/SDL3/include" ${SDL3_PKG_INCLUDE_DIRS})
        set(SDL3_LIBRARIES ${SDL3_PKG_LIBRARIES})
    elseif(SDL3_SYSTEM_LIBRARY)
        # System SDL3 library found by name; use the bundled public headers.
        set(SDL3_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/external/SDL3/include")
        set(SDL3_LIBRARIES "${SDL3_SYSTEM_LIBRARY}")
    else()
        # Fallback: expect the library in the SDL3 build tree
        set(SDL3_LIBRARIES "${SDL3_BUILD_DIR}/Release/SDL3.lib")
        set(SDL3_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/external/SDL3/include")
    endif()

    # For SDL3, headers live in the SDL3/ subdirectory of the include path.
    if(NOT SDL3_INCLUDE_DIRS)
        set(SDL3_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/external/SDL3/include")
    endif()

    # When SDL3 was built as a subdirectory, its generated headers (e.g.
    # SDL_revision.h) live in the build tree's include directory; append it
    # only when that build actually happened.
    if(SDL3_BUILD_DIR)
        set(SDL3_INCLUDE_DIRS "${SDL3_INCLUDE_DIRS}" "${SDL3_BUILD_DIR}/include")
    endif()

    # Set prefix for any legacy code that needs it
    set(SDL3_PREFIX "${CMAKE_SOURCE_DIR}/external/SDL3")
else(USE_INTERNAL_SDL3)
    # System SDL3 installation
    set(SDL3_PREFIX "/usr")
    if(APPLE)
        set(SDL3_PREFIX "/usr/local")
    endif(APPLE)
    set(SDL3_INCLUDE_DIRS "${SDL3_PREFIX}/include/SDL3" "${SDL3_PREFIX}/include")
    set(SDL3_LIBRARIES "-L${SDL3_PREFIX} -lSDL3")
endif(USE_INTERNAL_SDL3)

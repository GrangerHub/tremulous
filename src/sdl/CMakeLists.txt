include(${CMAKE_SOURCE_DIR}/cmake/SDL3.cmake)

add_library (
    sdl STATIC
    sdl_gamma.cpp
    sdl_glimp.cpp
    sdl_icon.h
    sdl_input.cpp
    sdl_snd.cpp
)

# Add SDL3 include directories for this module
# ${SDL3_INCLUDE_DIRS} includes external/SDL3/include (parent dir) so that
# #include <SDL3/SDL.h> resolves correctly. We do NOT add the SDL3/ subdirectory
# separately to avoid conflicts with old SDL2 headers at the include root.
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SDL3_INCLUDE_DIRS}
)

# Link against SDL3 libraries
target_link_libraries(sdl ${SDL3_LIBRARIES})

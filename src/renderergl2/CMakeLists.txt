include("${CMAKE_SOURCE_DIR}/cmake/SDL3.cmake")

find_package(OpenGL)

set(PARENT_DIR ${CMAKE_SOURCE_DIR}/src)
include_directories(
    ${PARENT_DIR}/jpeg-8c
    ${PARENT_DIR}/renderercommon 
    ${PARENT_DIR}/nettle-3.3
    ${SDL3_INCLUDE_DIRS} 
    )

set(GLSL_SHADERS
    glsl/bokeh_fp.glsl
    glsl/bokeh_vp.glsl
    glsl/calclevels4x_fp.glsl
    glsl/calclevels4x_vp.glsl
    glsl/depthblur_fp.glsl
    glsl/depthblur_vp.glsl
    glsl/dlight_fp.glsl
    glsl/dlight_vp.glsl
    glsl/down4x_fp.glsl
    glsl/down4x_vp.glsl
    glsl/fogpass_fp.glsl
    glsl/fogpass_vp.glsl
    glsl/generic_fp.glsl
    glsl/generic_vp.glsl
    glsl/lightall_fp.glsl
    glsl/lightall_vp.glsl
    glsl/pshadow_fp.glsl
    glsl/pshadow_vp.glsl
    glsl/shadowfill_fp.glsl
    glsl/shadowfill_vp.glsl
    glsl/shadowmask_fp.glsl
    glsl/shadowmask_vp.glsl
    glsl/ssao_fp.glsl
    glsl/ssao_vp.glsl
    glsl/texturecolor_fp.glsl
    glsl/texturecolor_vp.glsl
    glsl/tonemap_fp.glsl
    glsl/tonemap_vp.glsl
    glsl/gamma_fp.glsl
    glsl/gamma_vp.glsl
    )

# Converts ^^^GSLS shaders^^^ into const char* C-strings.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/glsl/)
foreach(shader ${GLSL_SHADERS})
    set(in ${CMAKE_CURRENT_SOURCE_DIR}/${shader})

    get_filename_component(out ${in} NAME_WE)
    set(out ${CMAKE_CURRENT_BINARY_DIR}/glsl/${out}.cpp)

    get_filename_component(shader ${shader} NAME_WE)
    message(STATUS "-- Generating embeded GLSL ${shader}")

    file(READ ${in} contents HEX)
    # Build the escaped hex string in memory, then write the entire C file
    # in ONE file(WRITE) call. The original code did file(APPEND) once per
    # byte (~23,000 calls for lightall_fp.glsl), which raced with Windows
    # Defender real-time scanning and produced "Permission denied" errors
    # during CMake configure on MSVC/VS2022 generators.
    set(escaped "")

    string(LENGTH "${contents}" contents_length)
    math(EXPR contents_length "${contents_length} - 1")

    foreach(iter RANGE 0 ${contents_length} 2)
      string(SUBSTRING ${contents} ${iter} 2 line)
      string(APPEND escaped "\\x${line}")
    endforeach()
    file(WRITE ${out} "const char *fallbackShader_${shader} = \"${escaped}\";\n")

    list(APPEND renderergl2_SHADERS "${out}")
    set_source_files_properties(${out} PROPERTIES GENERATED TRUE)
endforeach(shader)

set(renderergl2_SRCS
    ${renderergl2_SHADERS}
    tr_local.h
    tr_dsa.h
    tr_extramath.h
    tr_extratypes.h
    tr_fbo.h
    tr_postprocess.h
    tr_animation.cpp
    tr_backend.cpp
    tr_bsp.cpp
    tr_cmds.cpp
    tr_curve.cpp
    tr_dsa.cpp
    tr_extensions.cpp
    tr_extramath.cpp
    tr_fbo.cpp
    tr_flares.cpp
    tr_glsl.cpp
    tr_image.cpp
    tr_image_dds.cpp
    tr_init.cpp
    tr_light.cpp
    tr_main.cpp
    tr_marks.cpp
    tr_mesh.cpp
    tr_model.cpp
    tr_model_iqm.cpp
    tr_postprocess.cpp
    tr_scene.cpp
    tr_shade.cpp
    tr_shade_calc.cpp
    tr_shader.cpp
    tr_shadows.cpp
    tr_sky.cpp
    tr_subs.cpp
    tr_surface.cpp
    tr_vbo.cpp
    tr_world.cpp
    ${CMAKE_SOURCE_DIR}/src/qcommon/puff.cpp
    ${CMAKE_SOURCE_DIR}/src/qcommon/q_shared.c
    ${CMAKE_SOURCE_DIR}/src/qcommon/q_math.c
    )

if(NOT USE_RENDERER_DLOPEN)

    add_library(
        renderergl2 STATIC
        ${renderergl2_SRCS})

    target_link_libraries(
        renderergl2 renderercommon
        ${SDL3_LIBRARIES})

else(NOT USE_RENDERER_DLOPEN)

    add_library(
        renderergl2 SHARED
        ${renderergl2_SRCS}
        )

    # Match the canonical Makefile output name - the engine loads
    # renderer_opengl2.dll at runtime (see cl_main.cpp).
    set_target_properties(renderergl2 PROPERTIES OUTPUT_NAME renderer_opengl2)

    # When building as a DLL, USE_RENDERER_DLOPEN must be defined at compile
    # time so GetRefAPI gets extern "C" + __declspec(dllexport).  Without it
    # the symbol gets C++ name mangling and the client cannot find it via
    # Sys_LoadFunction(rendererLib, "GetRefAPI").
    target_compile_definitions(renderergl2 PRIVATE USE_RENDERER_DLOPEN)

    target_link_libraries(
        renderergl2
        renderercommon
        ${FRAMEWORKS}
        ${OPENGL_LIBRARIES}
        ${SDL3_LIBRARIES}
        )

endif(NOT USE_RENDERER_DLOPEN)


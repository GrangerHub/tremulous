
include(CMakeParseArguments)

# Use $<TARGET_FILE:...> so the path resolves correctly for both single-config
# (Makefiles) and multi-config (Visual Studio) generators. This avoids the
# per-config subdirectory mismatch (e.g. qvm_tools/Release/q3cpp.exe vs qvm_tools/q3cpp.exe).
set(Q3CPP_BINARY $<TARGET_FILE:q3cpp>)
set(Q3RCC_BINARY $<TARGET_FILE:q3rcc>)
set(Q3LCC_BINARY $<TARGET_FILE:q3lcc>)
set(Q3ASM_BINARY $<TARGET_FILE:q3asm>)

# Depend on the tool targets themselves (not file paths) so CMake builds them first.
set(QVM_DEPS q3asm q3cpp q3lcc q3rcc)

macro(QVM_COMPILE_ASM defs outfile infile)
    add_custom_command(
        OUTPUT  ${outfile}
        COMMAND ${Q3LCC_BINARY}
        ARGS    ${defs} -I${CMAKE_SOURCE_DIR}/src -o ${outfile} ${infile}
        DEPENDS ${QVM_DEPS}
        )
    set_source_files_properties(${outfile} PROPERTIES GENERATED TRUE)
endmacro()

macro(ADD_QVM Name)
    cmake_parse_arguments(ADD_QVM "" "" "" ${ARGN})

    #message( "QVM: ${Name}" )

    string(TOUPPER ${Name} UPNAME)
    set(defs "-DVMS -D${UPNAME}")

    #message( "DEFINITIONS ${defs}" )

    foreach(srcfile ${ADD_QVM_UNPARSED_ARGUMENTS})
        #message( "> ${srcfile}" )

        get_filename_component(ext ${srcfile} EXT) 
        if ("${ext}" STREQUAL ".asm")
            set(outfile ${CMAKE_CURRENT_SOURCE_DIR}/${srcfile})
            # Add asm files directly
            list(APPEND srcs ${outfile})
        endif()

        if ("${ext}" STREQUAL ".c")
            get_filename_component(outfile ${srcfile} NAME_WE)
            set(outfile ${outfile}.asm)
            # compile C code into asm
            qvm_compile_asm(${defs} ${outfile} ${CMAKE_CURRENT_SOURCE_DIR}/${srcfile})
            # add asm to list
            list(APPEND srcs ${outfile})
        endif()

        if ("${ext}" STREQUAL ".h")
            # XXX: Ignore headers??
        endif()
    endforeach()

    #message("SOURCES <${srcs}>")

    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/${Name}.qvm
        COMMAND ${Q3ASM_BINARY}
        ARGS -o ${Name}.qvm ${srcs}
        DEPENDS ${Q3ASM_BINARY} ${srcs}
        )
    add_custom_target(${Name}.qvm DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${Name}.qvm)
endmacro()
#
#============================================================

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(QVM 
    REQUIRED_VARS Q3LCC_BINARY Q3ASM_BINARY 
    )

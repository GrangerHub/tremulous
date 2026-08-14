# PackageQVMs.cmake - Script to find QVM files and package them into a .pk3
# Called as a custom command by the vms_pk3 target in the root CMakeLists.txt.
#
# Usage:
#   cmake -DBUILD_DIR=<build_dir> -DOUTPUT_DIR=<output_dir>
#         -DPK3_NAME=<name.pk3> -P cmake/PackageQVMs.cmake

# Derive the build root from OUTPUT_DIR's parent directory.
# OUTPUT_DIR is CMAKE_RUNTIME_OUTPUT_DIRECTORY (e.g., .../build/release-windows-amd64).
# The QVM files are in sibling directories under the build root (e.g., .../build/src/game/).
get_filename_component(BUILD_ROOT "${OUTPUT_DIR}" DIRECTORY)

# Search the entire build tree for QVM files (broad search to handle
# multi-config generators with platform/config subdirectories like x64/Release)
file(GLOB_RECURSE QVM_FILES
    "${BUILD_ROOT}/*.qvm"
)

message(STATUS "Searching for QVMs in: ${BUILD_ROOT}")
list(LENGTH QVM_FILES QVM_COUNT)
message(STATUS "Found ${QVM_COUNT} QVM files total")

# Find the best candidate for each QVM type.
# On multi-config generators, multiple copies may exist (e.g., game.qvm
# at both src/game/game.qvm and src/game/x64/Release/game.qvm).
# We prefer the one in a config subdirectory (it's the actual build output).
set(GAME_QVM "")
set(CGAME_QVM "")
set(UI_QVM "")

foreach(qvm ${QVM_FILES})
    get_filename_component(qvm_name ${qvm} NAME_WE)
    get_filename_component(qvm_dir ${qvm} DIRECTORY)

    if("${qvm_name}" STREQUAL "game")
        set(GAME_QVM ${qvm})
    elseif("${qvm_name}" STREQUAL "cgame")
        set(CGAME_QVM ${qvm})
    elseif("${qvm_name}" STREQUAL "ui")
        set(UI_QVM ${qvm})
    endif()
endforeach()

# Verify all QVMs were found
set(MISSING "")
if(NOT GAME_QVM)
    set(MISSING "${MISSING} game")
endif()
if(NOT CGAME_QVM)
    set(MISSING "${MISSING} cgame")
endif()
if(NOT UI_QVM)
    set(MISSING "${MISSING} ui")
endif()

if(MISSING)
    message(FATAL_ERROR "Missing QVM files:${MISSING}")
endif()

message(STATUS "Packaging QVMs:")
message(STATUS "  game:  ${GAME_QVM}")
message(STATUS "  cgame: ${CGAME_QVM}")
message(STATUS "  ui:    ${UI_QVM}")

# Create staging directory with vm/ subdirectory
set(VM_DIR "${OUTPUT_DIR}/vm")
file(REMOVE_RECURSE "${VM_DIR}")
file(MAKE_DIRECTORY "${VM_DIR}")

# Copy QVMs into vm/
file(COPY "${GAME_QVM}" DESTINATION "${VM_DIR}")
file(COPY "${CGAME_QVM}" DESTINATION "${VM_DIR}")
file(COPY "${UI_QVM}" DESTINATION "${VM_DIR}")

message(STATUS "Copied QVMs to ${VM_DIR}")

# Create the .pk3 (zip) file using cmake -E tar
# Working from OUTPUT_DIR so the archive contains vm/ as a subdirectory
set(PK3_PATH "${OUTPUT_DIR}/${PK3_NAME}")

# Remove existing pk3
file(REMOVE "${PK3_PATH}")

# Use cmake -E chdir + tar to create the zip with vm/ prefix
execute_process(
    COMMAND ${CMAKE_COMMAND} -E chdir "${OUTPUT_DIR}"
        ${CMAKE_COMMAND} -E tar cvf "${PK3_NAME}" --format=zip vm/
    RESULT_VARIABLE TAR_RESULT
)

if(TAR_RESULT)
    message(FATAL_ERROR "Failed to create pk3: ${PK3_NAME} (error: ${TAR_RESULT})")
endif()

if(EXISTS "${PK3_PATH}")
    message(STATUS "Created ${PK3_NAME}")
    message(STATUS "  at: ${PK3_PATH}")
else()
    message(FATAL_ERROR "Failed to create ${PK3_NAME}")
endif()

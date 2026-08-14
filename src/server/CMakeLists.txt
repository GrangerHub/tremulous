
#
##    .dMMMb  dMMMMMP dMMMMb  dMP dMP dMMMMMP dMMMMb
##   dMP" VP dMP     dMP.dMP dMP dMP dMP     dMP.dMP
##   VMMMb  dMMMP   dMMMMK" dMP dMP dMMMP   dMMMMK"
## dP .dMP dMP     dMP"AMF  YMvAP" dMP     dMP"AMF
## VMMMP" dMMMMMP dMP dMP    VP"  dMMMMMP dMP dMP
#

add_definitions(
    -DDEDICATED
    -DUSE_LOCAL_HEADERS
    -DPRODUCT_VERSION="${GAME_PRODUCT_VERSION}"
    -DPRODUCT_NAME="${GAME_PRODUCT_NAME}"
    -DUSE_VOIP
    -DNDEBUG
    )

set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/external)
set(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
if(APPLE)
set(APPLE_SOURCES ${PARENT_DIR}/sys/sys_osx.mm)
endif(APPLE)

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

# vm_x86.cpp contains GCC inline assembly (__asm__ volatile) for the JIT
# VM compiler. MSVC uses a different inline-asm syntax (__asm { ... }).
# Until the JIT is ported to MSVC, fall back to the interpreted VM only
# (vm_interpreted.cpp, already in the list) when building with MSVC.
if(NOT MSVC)
    list(APPEND VM_SOURCES ${PARENT_DIR}/qcommon/vm_x86.cpp)
endif()

add_executable(
    tremded
    #
    server.h
    #
    sv_ccmds.cpp
    sv_client.cpp
    sv_game.cpp
    sv_init.cpp
    sv_main.cpp
    sv_net_chan.cpp
    sv_snapshot.cpp
    sv_world.cpp
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
    ${PARENT_DIR}/qcommon/files.cpp
    ${PARENT_DIR}/qcommon/huffman.cpp
    ${PARENT_DIR}/qcommon/huffman.h
    ${PARENT_DIR}/qcommon/ioapi.cpp
    ${PARENT_DIR}/qcommon/md4.cpp
    ${PARENT_DIR}/qcommon/msg.h
    ${PARENT_DIR}/qcommon/msg.cpp
    ${PARENT_DIR}/qcommon/net.h
    ${PARENT_DIR}/qcommon/net_chan.cpp
    ${PARENT_DIR}/qcommon/net_ip.cpp
    ${PARENT_DIR}/qcommon/parse.cpp
    ${PARENT_DIR}/qcommon/q3_lauxlib.cpp
    ${PARENT_DIR}/qcommon/q_math.c
    ${PARENT_DIR}/qcommon/q_shared.c
    ${PARENT_DIR}/qcommon/unzip.cpp
    ${PARENT_DIR}/qcommon/vm.cpp
    ${PARENT_DIR}/qcommon/vm_interpreted.cpp
    # vm_x86.cpp (JIT VM compiler) only included when VM_SOURCES is set
    # (see above — excluded on MSVC because it uses GCC inline asm).
    ${VM_SOURCES}
    #
    ${PARENT_DIR}/null/null_client.cpp
    ${PARENT_DIR}/null/null_input.cpp
    ${PARENT_DIR}/null/null_snddma.cpp
    #
    # snapvector.c uses GCC inline asm (__asm__ volatile AT&T syntax).
    # Only include it when not building with MSVC (idx64/id386 are gated
    # to __GNUC__ in q_platform.h, so MSVC uses the portable C fallback).
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:${PARENT_DIR}/asm/snapvector.c>
    #
    ${PARENT_DIR}/sys/con_log.cpp
    ${PARENT_DIR}/sys/sys_main.cpp
    ${SYS_PLATFORM_SOURCES}
    ${PARENT_DIR}/sys/sys_shared.h
    ${APPLE_SOURCES}
    #
    ${EXTERNAL_DIR}/zlib/adler32.c
    ${EXTERNAL_DIR}/zlib/crc32.c
    ${EXTERNAL_DIR}/zlib/inffast.c
    ${EXTERNAL_DIR}/zlib/inflate.c
    ${EXTERNAL_DIR}/zlib/inftrees.c
    ${EXTERNAL_DIR}/zlib/zutil.c
    )

# MSVC x64 doesn't support inline assembly, so vm_x86.cpp is excluded.
# Define NO_VM_COMPILED to fall back to the bytecode interpreter.
if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    target_compile_definitions(tremded PRIVATE NO_VM_COMPILED)
endif()
if(APPLE)
 # FIXME Prefixed with "lua" to prevent cmake from doing "-l-framework Cocoa"
 set(FRAMEWORKS "-framework Cocoa -framework Security -framework OpenAL -framework IOKit")
elseif(WIN32)
 # Windows-specific libraries for networking and multimedia
 set(SYSLIBS ws2_32 winmm)
 elseif(UNIX)
   set(SYSLIBS dl rt)
 endif()

target_link_libraries(
    tremded
    #
    lua
    script_api
    nettle
    zlib
    ${FRAMEWORKS}
    ${SYSLIBS}
    )
    
include_directories(
    ${PARENT_DIR}/script
    ${EXTERNAL_DIR}/lua-5.3.3/include
    ${EXTERNAL_DIR}/sol
    ${EXTERNAL_DIR}/script/rapidjson
    ${EXTERNAL_DIR}/nettle-3.3
    ${EXTERNAL_DIR}/zlib
    )

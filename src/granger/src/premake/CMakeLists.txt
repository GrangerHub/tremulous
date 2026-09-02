if(NOT WIN32)
    add_definitions(-Wall -Wextra)
endif(NOT WIN32)

if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(WIN32)

# Add Lua include directory for premake
include_directories(${CMAKE_SOURCE_DIR}/external/lua-5.3.3/src)

# Add strvec functions used by premake
set(STRVEC_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../strvec.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../strvec.h
)

add_library(premake STATIC
	${STRVEC_SOURCES}
	os_access.c
	os_chdir.c
	os_copyfile.c
	os_elevate.c
	os_getcwd.c
	os_is64bit.c
	os_isdir.c
	os_isfile.c
	os_match.c
	os_mkdir.c
	os_pathsearch.c
	os_rmdir.c
	os_stat.c
	path_getabsolute.c
	path_getrelative.c
	path_isabsolute.c
	path_join.c
	path_normalize.c
	path_translate.c
	premake.c
	premake.h
	string_endswith.c
	)

# Link against Lua library for premake
target_link_libraries(premake granger_lua)

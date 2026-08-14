# Use CURL_INCLUDE_DIRS and CURL_LIBRARIES from root CMakeLists.txt
# These are set to point to static libcurl library and headers

add_library(
    restclient
    STATIC
    restclient/connection.h
    restclient/helpers.h
    restclient/restclient.h
    restclient/version.h
    connection.cpp
    helpers.cpp
    restclient.cpp
    )

# Define CURL_STATICLIB so curl headers don't use __declspec(dllimport)
target_compile_definitions(restclient PRIVATE CURL_STATICLIB)

# Force libcurl include directories for restclient
# Use the external libcurl headers directly
# Note: code uses #include "curl/curl.h" so we need to include the parent directory
target_include_directories(restclient PRIVATE ${CMAKE_SOURCE_DIR}/external/libcurl-7.35.0)

# Also add CURL_INCLUDE_DIRS if available
if(CURL_INCLUDE_DIRS)
    target_include_directories(restclient PRIVATE ${CURL_INCLUDE_DIRS})
endif()

# Link against static libcurl library
if(CURL_LIBRARIES)
    target_link_libraries(restclient PRIVATE ${CURL_LIBRARIES})
elseif(TARGET libcurl::libcurl)
    target_link_libraries(restclient PRIVATE libcurl::libcurl)
elseif(TARGET CURL::libcurl)
    target_link_libraries(restclient PRIVATE CURL::libcurl)
endif()

# Get libcurl include directories from the target
if(TARGET libcurl::libcurl)
    get_target_property(CURL_INCLUDE_DIRS libcurl::libcurl INTERFACE_INCLUDE_DIRECTORIES)
elseif(TARGET CURL::libcurl)
    get_target_property(CURL_INCLUDE_DIRS CURL::libcurl INTERFACE_INCLUDE_DIRECTORIES)
endif()

add_library(
    restclient
    restclient/connection.h
    restclient/helpers.h
    restclient/restclient.h
    restclient/version.h
    connection.cpp
    helpers.cpp
    restclient.cpp
    )

# Add libcurl include directories for restclient
if(CURL_INCLUDE_DIRS)
    target_include_directories(restclient PRIVATE ${CURL_INCLUDE_DIRS})
endif()

# Link against libcurl
target_link_libraries(restclient PRIVATE CURL_LIBRARIES)

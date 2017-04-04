cmake_minimum_required(VERSION 3.6)
project(utils)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        firmware.c
        vector.c
        vector.h)

find_package(PkgConfig REQUIRED)
pkg_search_module(OPENSSL REQUIRED openssl)

if( OPENSSL_FOUND )
    include_directories(${OPENSSL_INCLUDE_DIRS})
    message(STATUS "Using OpenSSL ${OPENSSL_VERSION}")
else()
    # Error; with REQUIRED, pkg_search_module() will throw an error by it's own
endif()


find_library(uuid /usr/local/lib)
find_path(UUID_INCLUDE_DIR uuid/uuid.h
        /usr/local/include
        /opt/local/include
        /usr/include
        )
include_directories(${UUID_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR})


add_executable(bgfirmware ${SOURCE_FILES})
target_link_libraries(bgfirmware ${OPENSSL_LIBRARIES})

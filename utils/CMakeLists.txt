cmake_minimum_required(VERSION 3.6)
project(utils)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        firmware.c
        vector.c
        vector.h)

find_library(uuid /usr/local/lib)
find_path(UUID_INCLUDE_DIR uuid/uuid.h
        /usr/local/include
        /opt/local/include
        /usr/include
        )
include_directories(${UUID_INCLUDE_DIR})


add_executable(bgfirmware ${SOURCE_FILES})
project(breeze)
cmake_minimum_required(VERSION 2.8.4)

set(CMAKE_C_FLAGS "$ENV{CFLAGS} -g -rdynamic -Wall")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
link_directories(${LIBRARY_OUTPUT_PATH})

include_directories(json)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE DEBUG)
endif ()

message(STATUS "Build type is ${CMAKE_BUILD_TYPE}")

aux_source_directory(. SRC)
add_subdirectory(tests)

add_executable(breeze ${SRC})

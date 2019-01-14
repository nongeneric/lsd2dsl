cmake_minimum_required (VERSION 3.11)

project(lsd2dsl)

set(lsd2dsl_VERSION "0.4.1")

configure_file (
    "${CMAKE_SOURCE_DIR}/version.h.in"
    "${CMAKE_BINARY_DIR}/version.h"
)

include_directories("${CMAKE_BINARY_DIR}")

set(CMAKE_CXX_STANDARD 14)

set(Boost_USE_STATIC_LIBS       OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.50 COMPONENTS system program_options filesystem REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

find_library(GTEST gtest)
find_library(MINIZIP minizip)
find_library(Z z)
find_library(VORBISFILE vorbisfile)
find_library(SNDFILE sndfile)

option(CMAKE_RELEASE "CMAKE_RELEASE" FALSE)

set(CMAKE_CXX_FLAGS "-Werror=return-type -Wall -Wextra -Werror -Wno-implicit-fallthrough ${CMAKE_CXX_FLAGS}")

if(CMAKE_RELEASE)
    add_definitions(-DNDEBUG)
    set(CMAKE_CXX_FLAGS "-O3 ${CMAKE_CXX_FLAGS}")
else()
    set(CMAKE_CXX_FLAGS "-O0 -ggdb ${CMAKE_CXX_FLAGS}")
endif()

add_subdirectory(dictlsd)

file(COPY simple_testdict1 DESTINATION .)

add_executable(lsd2dsl
    decoder.cpp
    ZipWriter.h
    ZipWriter.cpp
    DslWriter.h
    DslWriter.cpp
    version.h
)

if(NOT CMAKE_RELEASE)
    add_executable(tests tests.cpp)
    target_link_libraries(tests dictlsd ${GTEST})
endif()

target_link_libraries(lsd2dsl dictlsd ${MINIZIP})
add_subdirectory(qtgui)

if(WIN32)
    configure_file (
        "${CMAKE_SOURCE_DIR}/install.cmake.in"
        "${CMAKE_BINARY_DIR}/install.cmake"
        @ONLY
    )
    install(SCRIPT "${CMAKE_BINARY_DIR}/install.cmake")
    set(CPACK_PACKAGE_FILE_NAME "lsd2dsl-${lsd2dsl_VERSION}-win32")
    set(CPACK_GENERATOR "ZIP")
    include(CPack)
endif()

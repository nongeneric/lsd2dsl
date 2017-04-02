cmake_minimum_required (VERSION 3.1)

project(lsd2dsl)

set(CMAKE_CXX_STANDARD 14)

set(Boost_USE_STATIC_LIBS       OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.50 COMPONENTS system program_options filesystem REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

option(CMAKE_RELEASE "CMAKE_RELEASE" FALSE)

set(CMAKE_CXX_FLAGS "-Werror=return-type -Wall -Wextra -Werror -Wno-implicit-fallthrough ${CMAKE_CXX_FLAGS}")

if(CMAKE_RELEASE)
    add_definitions(-DNDEBUG)
    set(CMAKE_CXX_FLAGS "-O3 ${CMAKE_CXX_FLAGS}")
else()
    set(CMAKE_CXX_FLAGS "-O0 -g ${CMAKE_CXX_FLAGS}")
endif()

add_subdirectory(dictlsd)

FILE(COPY simple_testdict1 DESTINATION .)

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
    target_link_libraries(tests dictlsd gtest)
endif()

target_link_libraries(lsd2dsl dictlsd zip)
add_subdirectory(qtgui)

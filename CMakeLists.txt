cmake_minimum_required (VERSION 2.8.11)

project(lsd2dsl)

set(Boost_USE_STATIC_LIBS       OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.50 COMPONENTS system program_options filesystem REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

option(CMAKE_RELEASE "CMAKE_RELEASE" FALSE)

if(CMAKE_RELEASE)
    add_definitions(-DNDEBUG)
    set(CMAKE_CXX_FLAGS "-std=c++11 -O3 -Werror=return-type -Wall -Wextra -Werror ${CMAKE_CXX_FLAGS}")
else()
    set(CMAKE_CXX_FLAGS "-std=c++11 -O0 -g -Werror=return-type -Wall ${CMAKE_CXX_FLAGS}")
endif()

add_subdirectory(dictlsd)

FILE(COPY simple_testdict1 DESTINATION .)

add_executable(lsd2dsl
    decoder.cpp
    ZipWriter.h
    ZipWriter.cpp
    DslWriter.h
    DslWriter.cpp
)

if(NOT CMAKE_RELEASE)
    add_executable(tests tests.cpp)
    target_link_libraries(tests dictlsd gtest)
endif()

target_link_libraries(lsd2dsl dictlsd zip)
add_subdirectory(qtgui)

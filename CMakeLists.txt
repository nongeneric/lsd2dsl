cmake_minimum_required (VERSION 3.14)

project(lsd2dsl VERSION 0.5.5 LANGUAGES C CXX)

configure_file (
    "${CMAKE_SOURCE_DIR}/lib/common/version.h.in"
    "${CMAKE_BINARY_DIR}/lib/common/version.h"
)

include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${CMAKE_BINARY_DIR}")

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
    set(Boost_ARCHITECTURE "-x64")
endif()

set(Boost_USE_STATIC_LIBS       OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.60 COMPONENTS system program_options filesystem regex REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(-fno-limit-debug-info)
endif()

set(CMAKE_CXX_FLAGS "-Werror -Werror=return-type -Wall -Wextra -Wno-implicit-fallthrough ${CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS "-Wno-everything ${CMAKE_C_FLAGS}")

add_subdirectory(lib)
add_subdirectory(tests)
add_subdirectory(console)
add_subdirectory(gui)

if(WIN32)
    configure_file (
        "${CMAKE_SOURCE_DIR}/install.cmake.in"
        "${CMAKE_BINARY_DIR}/install.cmake"
        @ONLY
    )
    install(SCRIPT "${CMAKE_BINARY_DIR}/install.cmake")
    set(CPACK_PACKAGE_FILE_NAME "lsd2dsl-${lsd2dsl_VERSION}-win64")
    set(CPACK_GENERATOR "ZIP")
    include(CPack)
endif()

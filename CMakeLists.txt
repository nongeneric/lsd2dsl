cmake_minimum_required (VERSION 3.21)

project(lsd2dsl VERSION 0.6.0 LANGUAGES C CXX)

configure_file (
    "${CMAKE_SOURCE_DIR}/lib/common/version.h.in"
    "${CMAKE_BINARY_DIR}/lib/common/version.h"
)

include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${CMAKE_BINARY_DIR}")

set(CMAKE_CXX_STANDARD 17)

find_package(Boost 1.60 COMPONENTS system program_options REQUIRED)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Werror>)
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wno-error=array-bounds>)
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wall>)
if(WIN32)
    add_compile_definitions(WIN32)
endif()

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

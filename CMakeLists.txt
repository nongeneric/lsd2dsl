cmake_minimum_required (VERSION 3.21)

project(lsd2dsl VERSION 0.6.0 LANGUAGES C CXX)

option(ENABLE_DUDEN "Enable Duden support, requires Qt6WebEngine" ON)

configure_file (
    "${CMAKE_SOURCE_DIR}/lib/common/version.h.in"
    "${CMAKE_BINARY_DIR}/common/version.h"
)

include_directories("${PROJECT_SOURCE_DIR}/lib")
include_directories("${CMAKE_BINARY_DIR}")

set(CMAKE_CXX_STANDARD 20)

find_package(Boost 1.60 COMPONENTS system program_options REQUIRED)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Werror>)
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Wno-error=array-bounds>)
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Wall>)

add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-fno-char8_t>)
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Wno-deprecated-declarations>)
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/Zc:char8_t->)

if(WIN32)
    add_compile_definitions(WIN32)
endif()
if(ENABLE_DUDEN)
    add_compile_definitions(ENABLE_DUDEN)
endif()

add_subdirectory(lib)
add_subdirectory(tests)
add_subdirectory(app)

if(WIN32)
    if(ENABLE_DUDEN)
        set(CPACK_PACKAGE_FILE_NAME "lsd2dsl-${lsd2dsl_VERSION}-duden-win64")
    else()
        set(CPACK_PACKAGE_FILE_NAME "lsd2dsl-${lsd2dsl_VERSION}-win64")
    endif()
    configure_file (
        "${CMAKE_SOURCE_DIR}/install.cmake.in"
        "${CMAKE_BINARY_DIR}/install.cmake"
        @ONLY
    )
    install(SCRIPT "${CMAKE_BINARY_DIR}/install.cmake")
    set(CPACK_GENERATOR "ZIP")
    include(CPack)
endif()

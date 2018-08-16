cmake_minimum_required (VERSION 3.11)

project(lsd2dsl)

set(lsd2dsl_VERSION "0.3.1")

configure_file (
    "${lsd2dsl_SOURCE_DIR}/version.h.in"
    "${lsd2dsl_BINARY_DIR}/version.h"
)

include_directories("${lsd2dsl_BINARY_DIR}")

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
    set(CMAKE_CXX_FLAGS "-O0 -ggdb ${CMAKE_CXX_FLAGS}")
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

if(WIN32)
    install(TARGETS lsd2dsl RUNTIME DESTINATION .)

    include(GetPrerequisites)

    set(MINGW_ROOT /usr/i686-w64-mingw32/sys-root/mingw)

    set(gp_tool objdump)
    get_prerequisites("${lsd2dsl_BINARY_DIR}/lsd2dsl.exe" prereq 0 1 "" "${MINGW_ROOT}/bin")
    get_prerequisites("${qtgui_BINARY_DIR}/lsd2dsl-qtgui.exe" prereq 0 1 "" "${MINGW_ROOT}/bin")
    get_prerequisites("${MINGW_ROOT}/lib/qt5/plugins/platforms/qwindows.dll" prereq 0 1 "" "${MINGW_ROOT}/bin")
    foreach(dll IN LISTS prereq)
        set(path "/usr/i686-w64-mingw32/sys-root/mingw/bin/${dll}")
        if(EXISTS ${path})
            install(FILES ${path} DESTINATION .)
        endif()
    endforeach()

    install(FILES
        ${MINGW_ROOT}/lib/qt5/plugins/platforms/qwindows.dll
        DESTINATION platforms
    )

    install(CODE "
        file(GLOB_RECURSE files \"\${CMAKE_INSTALL_PREFIX}/*dll\"
                                \"\${CMAKE_INSTALL_PREFIX}/*exe\")
        foreach(binary IN LISTS files)
            execute_process(COMMAND mingw-strip \"-s\" \${binary})
        endforeach()
    ")

    set(CPACK_PACKAGE_FILE_NAME "lsd2dsl-${lsd2dsl_VERSION}-win32")
    set(CPACK_GENERATOR "ZIP")
    include(CPack)
endif()

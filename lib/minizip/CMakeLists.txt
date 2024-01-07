project(minizip LANGUAGES C)

find_package(ZLIB REQUIRED)

set(SOURCES
    ioapi.c
    miniunz.c
    minizip.c
    mztools.c
    unzip.c
    zip.c
)

if(WIN32)
    list(APPEND SOURCES iowin32.c)
endif()

add_library(${PROJECT_NAME} STATIC ${SOURCES}
    ioapi.c
    miniunz.c
    minizip.c
    mztools.c
    unzip.c
    zip.c
)

target_link_libraries(${PROJECT_NAME}
    ZLIB::ZLIB
)

project(minizip)

set(SOURCES
    crypt.h
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
    crypt.h
    ioapi.c
    miniunz.c
    minizip.c
    mztools.c
    unzip.c
    zip.c
)
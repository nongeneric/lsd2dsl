project(common)

find_package(Qt6 REQUIRED COMPONENTS Core Core5Compat)

find_package(fmt CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
if(WIN32)
    find_package(SndFile CONFIG REQUIRED)
else()
    find_library(SNDFILE sndfile REQUIRED)
    add_library(SndFile::sndfile SHARED IMPORTED)
    set_property(TARGET SndFile::sndfile PROPERTY IMPORTED_LOCATION ${SNDFILE})
endif()

add_library(${PROJECT_NAME} STATIC
    BitStream.cpp
    CommonTools.cpp
    DslWriter.cpp
    Log.cpp
    WavWriter.cpp
    ZipWriter.cpp
)

target_link_libraries(${PROJECT_NAME}
    Boost::system
    Qt6::Core
    Qt6::Core5Compat
    minizip
    SndFile::sndfile
    fmt::fmt-header-only
    ZLIB::ZLIB
)

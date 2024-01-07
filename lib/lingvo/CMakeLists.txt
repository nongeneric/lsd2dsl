project(lingvo)

find_package(Qt6 REQUIRED COMPONENTS Core Core5Compat)

find_package(fmt CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
if(WIN32)
    find_package(Vorbis CONFIG REQUIRED)
else()
    find_library(VORBISFILE vorbisfile REQUIRED)
    add_library(Vorbis::vorbisfile SHARED IMPORTED)
    set_property(TARGET Vorbis::vorbisfile PROPERTY IMPORTED_LOCATION ${VORBISFILE})
endif()

add_library(${PROJECT_NAME} STATIC
    AbbreviationDictionaryDecoder.cpp
    ArticleHeading.cpp
    CachePage.cpp
    DictionaryReader.cpp
    IDictionaryDecoder.cpp
    LenTable.cpp
    LSAReader.cpp
    lsd.cpp
    LSDOverlayReader.cpp
    OggReader.cpp
    SystemDictionaryDecoder.cpp
    tools.cpp
    UserDictionaryDecoder.cpp
    WriteDsl.cpp
)

target_link_libraries(${PROJECT_NAME}
    common
    Boost::system
    Qt6::Core
    Qt6::Core5Compat
    Vorbis::vorbisfile
    fmt::fmt-header-only
    ZLIB::ZLIB
)

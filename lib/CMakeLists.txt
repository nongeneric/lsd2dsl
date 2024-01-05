project(lib)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets WebEngineWidgets Core5Compat Pdf)

find_package(fmt CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
if(WIN32)
    find_package(Vorbis CONFIG REQUIRED)
    find_package(SndFile CONFIG REQUIRED)
else()
    find_library(VORBISFILE vorbisfile REQUIRED)
    add_library(Vorbis::vorbisfile SHARED IMPORTED)
    set_property(TARGET Vorbis::vorbisfile PROPERTY IMPORTED_LOCATION ${VORBISFILE})

    find_library(SNDFILE sndfile REQUIRED)
    add_library(SndFile::sndfile SHARED IMPORTED)
    set_property(TARGET SndFile::sndfile PROPERTY IMPORTED_LOCATION ${SNDFILE})
endif()

add_subdirectory(minizip)

add_library(${PROJECT_NAME} STATIC
    lsd/lsd.cpp
    lsd/tools.cpp
    lsd/LenTable.cpp
    lsd/BitStream.cpp
    lsd/ArticleHeading.cpp
    lsd/CachePage.cpp
    lsd/DictionaryReader.cpp
    lsd/IDictionaryDecoder.cpp
    lsd/UserDictionaryDecoder.cpp
    lsd/SystemDictionaryDecoder.cpp
    lsd/AbbreviationDictionaryDecoder.cpp
    lsd/LSDOverlayReader.cpp
    lsd/LSAReader.cpp
    lsd/OggReader.cpp
    duden/Duden.cpp
    duden/Archive.cpp
    duden/Dictionary.cpp
    duden/InfFile.cpp
    duden/Writer.cpp
    duden/LdFile.cpp
    duden/HtmlRenderer.cpp
    duden/TableRenderer.cpp
    duden/IFileSystem.cpp
    duden/AdpDecoder.cpp
    duden/FsdFile.cpp
    duden/text/TextRun.cpp
    duden/text/Table.cpp
    duden/text/Parser.cpp
    duden/text/Printers.cpp
    duden/text/Reference.cpp
    duden/unzip/inflate.c
    common/ZipWriter.cpp
    common/DslWriter.cpp
    common/WavWriter.cpp
    common/Log.cpp
)

target_link_libraries(${PROJECT_NAME}
    Boost::system
    Qt6::Core
    Qt6::Core5Compat
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::Pdf
    minizip
    Vorbis::vorbisfile
    SndFile::sndfile
    fmt::fmt-header-only
    ZLIB::ZLIB
)

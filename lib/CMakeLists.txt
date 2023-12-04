project(lib)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets WebEngineWidgets Core5Compat)

find_package(fmt CONFIG REQUIRED)
find_library(Z z)
find_library(VORBISFILE vorbisfile)
find_library(SNDFILE sndfile)

add_subdirectory(minizip)

add_library(${PROJECT_NAME} STATIC
    lsd/lsd.h
    lsd/lsd.cpp
    lsd/tools.h
    lsd/tools.cpp
    lsd/LenTable.h
    lsd/LenTable.cpp
    lsd/BitStream.h
    lsd/BitStream.cpp
    lsd/ArticleHeading.h
    lsd/ArticleHeading.cpp
    lsd/CachePage.h
    lsd/CachePage.cpp
    lsd/DictionaryReader.h
    lsd/DictionaryReader.cpp
    lsd/IDictionaryDecoder.h
    lsd/IDictionaryDecoder.cpp
    lsd/UserDictionaryDecoder.h
    lsd/UserDictionaryDecoder.cpp
    lsd/SystemDictionaryDecoder.h
    lsd/SystemDictionaryDecoder.cpp
    lsd/AbbreviationDictionaryDecoder.h
    lsd/AbbreviationDictionaryDecoder.cpp
    lsd/LSDOverlayReader.h
    lsd/LSDOverlayReader.cpp
    lsd/LSAReader.h
    lsd/LSAReader.cpp
    lsd/OggReader.h
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
    common/ZipWriter.h
    common/ZipWriter.cpp
    common/DslWriter.h
    common/DslWriter.cpp
    common/WavWriter.h
    common/WavWriter.cpp
    common/Log.h
    common/Log.cpp
    common/version.h
)

target_link_libraries(${PROJECT_NAME}
    ${Boost_LIBRARIES}
    Qt6::Core
    Qt6::Core5Compat
    Qt6::Widgets
    Qt6::WebEngineWidgets
    minizip
    ${Z}
    ${VORBISFILE}
    ${SNDFILE}
    fmt::fmt-header-only
)

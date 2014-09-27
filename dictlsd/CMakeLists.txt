cmake_minimum_required(VERSION 2.8)

project(dictlsd)

set(SRC_LIST
    lsd.h
    lsd.cpp
    tools.h
    tools.cpp
    LenTable.h
    LenTable.cpp
    BitStream.h
    BitStream.cpp
    ArticleHeading.h
    ArticleHeading.cpp
    CachePage.h
    CachePage.cpp
    DictionaryReader.h
    DictionaryReader.cpp
    IDictionaryDecoder.h
    IDictionaryDecoder.cpp
    UserDictionaryDecoder.h
    UserDictionaryDecoder.cpp
    SystemDictionaryDecoder.h
    SystemDictionaryDecoder.cpp
    AbbreviationDictionaryDecoder.h
    AbbreviationDictionaryDecoder.cpp
    LSDOverlayReader.h
    LSDOverlayReader.cpp
    LSAReader.h
    LSAReader.cpp
    OggReader.h
    OggReader.cpp
    WavWriter.h
    WavWriter.cpp
)
add_library(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} z vorbisfile sndfile)

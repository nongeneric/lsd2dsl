project(duden)

find_package(Qt5Core)

set(SRC_LIST
    Duden.cpp
    Archive.cpp
    Dictionary.cpp
    InfFile.cpp
    Writer.cpp
    LdFile.cpp
    HtmlRenderer.cpp
    TableRenderer.cpp
    IFileSystem.cpp
    text/TextRun.cpp
    text/Table.cpp
    text/Parser.cpp
    text/Printers.cpp
    text/Reference.cpp
    unzip/inflate.c
)

add_library(${PROJECT_NAME} STATIC ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} Qt5::Core ${Boost_LIBRARIES} ${Z})

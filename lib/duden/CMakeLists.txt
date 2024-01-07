project(duden)

find_package(Qt6 REQUIRED COMPONENTS WebEngineWidgets Pdf)

find_package(fmt CONFIG REQUIRED)

add_library(${PROJECT_NAME} STATIC
    Duden.cpp
    Archive.cpp
    Dictionary.cpp
    InfFile.cpp
    Writer.cpp
    LdFile.cpp
    HtmlRenderer.cpp
    TableRenderer.cpp
    IFileSystem.cpp
    AdpDecoder.cpp
    FsdFile.cpp
    text/TextRun.cpp
    text/Table.cpp
    text/Parser.cpp
    text/Printers.cpp
    text/Reference.cpp
    unzip/inflate.c
)

target_link_libraries(${PROJECT_NAME}
    common
    Boost::system
    Qt6::WebEngineWidgets
    Qt6::Pdf
    fmt::fmt-header-only
)

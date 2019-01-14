cmake_minimum_required(VERSION 3.11)

project(qtgui)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if(WIN32)
    set(CMAKE_CXX_FLAGS "-Wl,-subsystem,windows ${CMAKE_CXX_FLAGS}")
endif()

find_package(Qt5Widgets)
find_package(Qt5Core)
set(SRC
    main.cpp
    MainWindow.h
    MainWindow.cpp
    ../ZipWriter.h
    ../ZipWriter.cpp
    ../DslWriter.h
    ../DslWriter.cpp
)
add_executable(lsd2dsl-qtgui ${SRC} ${QT_RESOURCES})
target_link_libraries(lsd2dsl-qtgui Qt5::Core Qt5::Widgets dictlsd ${MINIZIP})

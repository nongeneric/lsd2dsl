cmake_minimum_required(VERSION 3.11)

project(qtgui)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if(WIN32)
    set(CMAKE_CXX_FLAGS "-Wl,-subsystem,windows ${CMAKE_CXX_FLAGS}")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Pdf)
set(SRC
    main.cpp
    MainWindow.cpp
)
add_executable(lsd2dsl-qtgui ${SRC} ${QT_RESOURCES})
target_link_libraries(lsd2dsl-qtgui PRIVATE Qt6::Core Qt6::Widgets Qt6::Pdf lib)

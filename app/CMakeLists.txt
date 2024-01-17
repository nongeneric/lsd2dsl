project(lsd2dsl)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
set(SRC
    decoder.cpp
    MainWindow.cpp
)
add_executable(${PROJECT_NAME} WIN32 ${SRC} ${QT_RESOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE lingvo Qt6::Core Qt6::Widgets Boost::program_options)

if(ENABLE_DUDEN)
    target_link_libraries(${PROJECT_NAME} PRIVATE duden)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION /)
install(FILES $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> DESTINATION /)

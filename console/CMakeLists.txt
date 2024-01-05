project(lsd2dsl)

add_executable(${PROJECT_NAME} decoder.cpp)
target_link_libraries(${PROJECT_NAME} lib Boost::program_options)

install(TARGETS ${PROJECT_NAME} DESTINATION /)
install(FILES $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> DESTINATION /)

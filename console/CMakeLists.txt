project(lsd2dsl)

add_executable(${PROJECT_NAME} decoder.cpp)
target_link_libraries(${PROJECT_NAME} lingvo Boost::program_options)
if(ENABLE_DUDEN)
    target_link_libraries(${PROJECT_NAME} duden)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION /)
install(FILES $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> DESTINATION /)

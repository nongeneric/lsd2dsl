project(lsd2dsl)

add_executable(${PROJECT_NAME} decoder.cpp)
target_link_libraries(${PROJECT_NAME} lib)

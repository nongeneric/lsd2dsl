project(lsd2dsl)

add_executable(${PROJECT_NAME} decoder.cpp)
target_link_libraries(${PROJECT_NAME} lib Boost::program_options)
if(WIN32)
    target_link_options(${PROJECT_NAME} PUBLIC -municode)
endif()

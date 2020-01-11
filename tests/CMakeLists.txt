project(tests)

find_library(GTEST gtest)

add_executable(tests tests.cpp duden-tests.cpp)
target_link_libraries(tests lib ${GTEST})

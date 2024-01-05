project(tests)

find_package(GTest CONFIG REQUIRED)

add_executable(tests tests.cpp duden-tests.cpp)
target_link_libraries(tests lib GTest::gtest)

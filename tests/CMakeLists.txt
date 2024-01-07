find_package(GTest CONFIG REQUIRED)

add_executable(lingvo-tests lingvo-tests.cpp)
target_link_libraries(lingvo-tests lingvo GTest::gtest_main)

if(ENABLE_DUDEN)
    add_executable(duden-tests duden-tests.cpp)
    target_link_libraries(duden-tests common duden GTest::gtest_main)
endif()

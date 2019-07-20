project(tests)

find_library(GTEST gtest)

file(COPY simple_testdict1 DESTINATION .)
file(COPY duden_testfiles DESTINATION .)

add_executable(tests tests.cpp duden-tests.cpp)
target_link_libraries(tests dictlsd duden ${GTEST} Qt5::WebKitWidgets)

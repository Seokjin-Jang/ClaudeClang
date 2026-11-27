find_package(GTest REQUIRED)
add_executable(tdd_tests test_tdd.cpp)
target_include_directories(tdd_tests PRIVATE ${CMAKE_SOURCE_DIR}/tests/support)
target_link_libraries(tdd_tests PRIVATE eclp GTest::gtest_main)

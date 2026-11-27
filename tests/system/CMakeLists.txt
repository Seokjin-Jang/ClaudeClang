find_package(GTest REQUIRED)
add_executable(system_tests test_system.cpp)
target_include_directories(system_tests PRIVATE ${CMAKE_SOURCE_DIR}/tests/support)
target_link_libraries(system_tests PRIVATE eclp GTest::gtest_main)

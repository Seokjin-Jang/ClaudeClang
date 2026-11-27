find_package(GTest REQUIRED)
add_executable(integration_tests test_integration.cpp)
target_include_directories(integration_tests PRIVATE ${CMAKE_SOURCE_DIR}/tests/support)
target_link_libraries(integration_tests PRIVATE eclp GTest::gtest_main)

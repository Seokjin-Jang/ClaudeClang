find_package(GTest REQUIRED)
add_executable(unit_tests
    test_validator.cpp
    test_state.cpp
    test_arbiter.cpp
    test_controller.cpp)
target_include_directories(unit_tests PRIVATE ${CMAKE_SOURCE_DIR}/tests/support)
target_link_libraries(unit_tests PRIVATE eclp GTest::gtest_main)

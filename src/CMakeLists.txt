add_library(eclp STATIC
    input_validator.c
    state_manager.c
    arbiter.c
    controller.c
    result_format.c)
target_include_directories(eclp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(eclp PUBLIC m)

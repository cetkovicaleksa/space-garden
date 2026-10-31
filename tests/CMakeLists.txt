add_executable(core_tests core_tests.cpp)
target_link_libraries(core_tests PRIVATE garden_core)
add_test(NAME core_tests COMMAND core_tests)

add_executable(garden_cli garden_cli.cpp)
target_link_libraries(garden_cli PRIVATE garden_core)

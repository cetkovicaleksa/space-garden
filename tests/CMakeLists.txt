add_executable(core_tests core_tests.cpp)
target_link_libraries(core_tests PRIVATE garden_core)
add_test(NAME core_tests COMMAND core_tests)

add_executable(garden_cli garden_cli.cpp)
target_link_libraries(garden_cli PRIVATE garden_core)

# Ray picking math of the 3D app (header-only, no OpenGL).
add_executable(picking_tests picking_tests.cpp)
target_include_directories(picking_tests PRIVATE ${PROJECT_SOURCE_DIR}/app3d)
target_link_libraries(picking_tests PRIVATE vendor_headers)
add_test(NAME picking_tests COMMAND picking_tests)

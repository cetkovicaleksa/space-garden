add_executable(space_garden_3d
    main.cpp
    GpuMesh.cpp
    OrbitCamera.cpp
    Primitives.cpp)
target_link_libraries(space_garden_3d PRIVATE garden_engine)
if(NOT MSVC)
    target_compile_options(space_garden_3d PRIVATE -Wall -Wextra)
endif()

if(WIN32)
    # GUI subsystem: no console window behind the fullscreen app.
    set_target_properties(space_garden_3d PROPERTIES WIN32_EXECUTABLE ON)
endif()

install(TARGETS space_garden_3d RUNTIME DESTINATION .)

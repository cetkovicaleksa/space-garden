add_executable(space_garden_2d
    main.cpp
    Renderer2D.cpp)
target_link_libraries(space_garden_2d PRIVATE garden_engine)

if(WIN32)
    # GUI subsystem: no console window behind the fullscreen app.
    set_target_properties(space_garden_2d PROPERTIES WIN32_EXECUTABLE ON)
endif()

install(TARGETS space_garden_2d RUNTIME DESTINATION .)

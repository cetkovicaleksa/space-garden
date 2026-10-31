# garden_engine: OpenGL/GLFW helpers shared by the 2D and 3D apps.
add_library(garden_engine STATIC
    src/NameTag.cpp
    src/Paths.cpp
    src/SpriteRenderer.cpp
    src/Texture.cpp
    src/Window.cpp)
target_include_directories(garden_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(garden_engine PUBLIC garden_core glad glfw vendor_headers)
target_compile_definitions(garden_engine PRIVATE SPACE_GARDEN_ASSETS_DIR="${PROJECT_SOURCE_DIR}/assets")
if(NOT MSVC)
    target_compile_options(garden_engine PRIVATE -Wall -Wextra)
endif()

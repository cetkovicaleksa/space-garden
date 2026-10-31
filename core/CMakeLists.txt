# garden_core: pure C++ logic model. Must not depend on OpenGL or GLFW.
add_library(garden_core STATIC
    src/Garden.cpp
    src/Names.cpp)
target_include_directories(garden_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(garden_core PUBLIC cxx_std_17)
if(MSVC)
    target_compile_options(garden_core PRIVATE /W4)
else()
    target_compile_options(garden_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

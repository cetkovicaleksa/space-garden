cmake_minimum_required(VERSION 3.20)
project(space_garden VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

enable_testing()

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# --- GLFW 3.4 ---------------------------------------------------------------
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(glfw)

# --- Vendored headers (glad, KHR, glm, stb_image, learnopengl) --------------
add_library(vendor_headers INTERFACE)
target_include_directories(vendor_headers SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# --- GLAD (OpenGL 3.3 core loader) ------------------------------------------
add_library(glad STATIC third_party/glad/glad.c)
target_link_libraries(glad PUBLIC vendor_headers)

add_subdirectory(core)
add_subdirectory(tests)
add_subdirectory(app2d)

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

# --- FreeType 2.13 (text rendering) ----------------------------------------
set(FT_DISABLE_ZLIB     ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2    ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG      ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI   ON CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL    ON CACHE BOOL "" FORCE)
FetchContent_Declare(freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG        VER-2-13-3
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(freetype)

# --- Assimp 5.4 (plant models in the 3D app; only the OBJ importer is built) --------------
set(BUILD_SHARED_LIBS                     OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS                    OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS             OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES                  OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL                        OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX           OFF CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS             OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT                      ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB                     ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER             ON  CACHE BOOL "" FORCE)
FetchContent_Declare(assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG        v5.4.3
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(assimp)

# --- Vendored headers (glad, KHR, glm, stb_image, learnopengl) --------------
add_library(vendor_headers INTERFACE)
target_include_directories(vendor_headers SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# --- GLAD (OpenGL 3.3 core loader) ------------------------------------------
add_library(glad STATIC third_party/glad/glad.c)
target_link_libraries(glad PUBLIC vendor_headers)

option(SPACE_GARDEN_STATIC_RUNTIME "Statically link the C++ runtime (portable release binaries)" OFF)
if(SPACE_GARDEN_STATIC_RUNTIME AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    add_link_options(-static-libgcc -static-libstdc++)
    if(MINGW)
        add_link_options(-static)
    endif()
endif()

add_subdirectory(core)
add_subdirectory(tests)
add_subdirectory(engine)
add_subdirectory(app2d)
add_subdirectory(app3d)

# Release package: the app plus assets/ next to it (found at runtime relative to the executable).
install(DIRECTORY assets/ DESTINATION assets)
install(FILES README.md DESTINATION .)

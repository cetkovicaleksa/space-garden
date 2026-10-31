#pragma once

#include <string>

namespace engine {

// Absolute path of a file inside assets/. The directory is baked in by CMake (SPACE_GARDEN_ASSETS_DIR)
// so the apps work from any working directory; SPACE_GARDEN_ASSETS overrides it at runtime.
std::string assetPath(const std::string& relative);

inline std::string shaderPath(const std::string& name) { return assetPath("shaders/" + name); }

} // namespace engine

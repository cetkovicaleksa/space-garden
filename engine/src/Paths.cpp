#include "engine/Paths.h"

#include <cstdlib>

#ifndef SPACE_GARDEN_ASSETS_DIR
#define SPACE_GARDEN_ASSETS_DIR "assets"
#endif

namespace engine {

std::string assetPath(const std::string& relative)
{
    const char* env = std::getenv("SPACE_GARDEN_ASSETS");
    std::string base = (env && env[0]) ? env : SPACE_GARDEN_ASSETS_DIR;
    if (!base.empty() && base.back() != '/')
        base += '/';
    return base + relative;
}

} // namespace engine

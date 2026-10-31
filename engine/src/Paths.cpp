#include "engine/Paths.h"

#include <cstdlib>
#include <filesystem>
#include <system_error>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#ifndef SPACE_GARDEN_ASSETS_DIR
#define SPACE_GARDEN_ASSETS_DIR "assets"
#endif

namespace engine {

namespace fs = std::filesystem;

namespace {

fs::path executableDir()
{
    std::error_code ec;
#ifdef _WIN32
    wchar_t buffer[MAX_PATH];
    const DWORD n = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (n == 0 || n == MAX_PATH)
        return {};
    return fs::path(buffer).parent_path();
#else
    const fs::path exe = fs::read_symlink("/proc/self/exe", ec);
    return ec ? fs::path() : exe.parent_path();
#endif
}

// Resolution order: SPACE_GARDEN_ASSETS, assets/ next to the executable (release packages),
// then the source tree baked in by CMake (development builds).
std::string findAssetsDir()
{
    if (const char* env = std::getenv("SPACE_GARDEN_ASSETS"); env && env[0])
        return env;
    std::error_code ec;
    const fs::path exeDir = executableDir();
    if (!exeDir.empty() && fs::is_directory(exeDir / "assets" / "shaders", ec))
        return (exeDir / "assets").string();
    return SPACE_GARDEN_ASSETS_DIR;
}

} // namespace

std::string assetPath(const std::string& relative)
{
    static const std::string base = [] {
        std::string b = findAssetsDir();
        if (!b.empty() && b.back() != '/' && b.back() != '\\')
            b += '/';
        return b;
    }();
    return base + relative;
}

} // namespace engine

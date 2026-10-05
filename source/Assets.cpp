#include "Assets.hpp"
#include "Error.hpp"
#include <fstream>
#include <memory>
#include <png.h>
#include <string>

namespace voxel {
Image loadPng(const char* path) {
    // Simplified libpng API contains its own setjmp boundary; C++ objects are safe.
    png_image png{};
    png.version = PNG_IMAGE_VERSION;
    struct Cleanup {
        png_image* png;
        ~Cleanup() { png_image_free(png); }
    } cleanup{&png};
    if (!png_image_begin_read_from_file(&png, path))
        throw Error("PNG '%s': %s", path, png.message);
    if (!png.width || !png.height || png.width > 1024 || png.height > 1024)
        throw Error("PNG '%s': dimensions %lux%lu exceed the 1024x1024 decode limit", path,
                    static_cast<unsigned long>(png.width), static_cast<unsigned long>(png.height));
    png.format = PNG_FORMAT_RGBA;
    Image image{png.width, png.height, {}};
    image.rgba.resize(PNG_IMAGE_SIZE(png));
    if (!png_image_finish_read(&png, nullptr, image.rgba.data(), 0, nullptr))
        throw Error("PNG '%s': %s", path, png.message);
    return image; // CPU row-major RGBA; not yet a PICA tiled texture.
}
nlohmann::json loadJson(const char* path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        throw Error("JSON '%s': unable to open file", path);
    const auto size = file.tellg();
    if (size < 0 || size > 1024 * 1024)
        throw Error("JSON '%s': invalid size or exceeds 1 MiB limit", path);
    std::string data(static_cast<std::size_t>(size), '\0');
    file.seekg(0);
    if (!file.read(data.data(), static_cast<std::streamsize>(data.size())))
        throw Error("JSON '%s': read failed", path);
    try {
        // Cap nesting before recursive DOM parsing can exhaust a 3DS thread stack.
        auto depthLimit = [](int depth, nlohmann::json::parse_event_t, nlohmann::json&) {
            if (depth > 32)
                throw Error("JSON nesting exceeds 32 levels");
            return true;
        };
        return nlohmann::json::parse(data, depthLimit);
    } catch (const std::bad_alloc&) {
        throw;
    } catch (const std::exception& e) {
        throw Error("JSON '%s': %s", path, e.what());
    }
}
} // namespace voxel

#include "AssetArchive.hpp"
#include "Error.hpp"
#include <fstream>
#include <png.h>
namespace voxel {
void AssetArchive::preload(const std::string& root, const Progress& progress) {
    AssetArchive next;
    std::vector<std::pair<std::string, unsigned>> pending{{"", 0}};
    unsigned entries = 0;
    while (!pending.empty()) {
        auto current = pending.back();
        pending.pop_back();
        if (current.second > 24)
            throw Error("Asset directory depth exceeds 24: %s", current.first.c_str());
        next.directories_.push_back(current.first);
        for (const auto& e :
             listDirectory(root + (current.first.empty() ? "" : "/" + current.first))) {
            if (++entries > 16384)
                throw Error("Asset tree exceeds 16384 entries");
            const auto path = current.first.empty() ? e.name : current.first + "/" + e.name;
            if (!safeRelativePath(path))
                throw Error("Invalid asset path '%s'", path.c_str());
            if (e.directory) {
                pending.push_back({path, current.second + 1});
                continue;
            }
            if (progress)
                progress("Caching " + path, unsigned(next.bytes_ / 1024), 0);
            std::ifstream in(root + "/" + path, std::ios::binary | std::ios::ate);
            if (!in)
                throw Error("Cannot preload '%s'", path.c_str());
            const auto size = in.tellg();
            if (size < 0 || std::uint64_t(size) > MaxBytes - next.bytes_)
                throw Error("Asset RAM budget (32 MiB) exceeded at '%s'", path.c_str());
            auto& data = next.files_[path];
            data.resize(std::size_t(size));
            in.seekg(0);
            if (!data.empty() && !in.read(reinterpret_cast<char*>(data.data()), data.size()))
                throw Error("Asset read failed '%s'", path.c_str());
            next.bytes_ += data.size();
        }
    }
    *this = std::move(next);
    if (progress)
        progress("Assets cached in RAM", 1, 1);
}
const std::vector<std::uint8_t>& AssetArchive::file(const std::string& path) const {
    auto it = files_.find(path);
    if (it == files_.end())
        throw Error("Missing cached asset '%s'", path.c_str());
    return it->second;
}
std::vector<std::string> AssetArchive::list(const std::string& prefix) const {
    std::vector<std::string> result;
    auto it = files_.lower_bound(prefix);
    for (; it != files_.end() && it->first.compare(0, prefix.size(), prefix) == 0; ++it)
        result.push_back(it->first.substr(prefix.size()));
    return result;
}
Image AssetArchive::png(const std::string& path) const {
    const auto& bytes = file(path);
    png_image png{};
    png.version = PNG_IMAGE_VERSION;
    struct Guard {
        png_image* p;
        ~Guard() { png_image_free(p); }
    } guard{&png};
    if (!png_image_begin_read_from_memory(&png, bytes.data(), bytes.size()))
        throw Error("PNG '%s': %s", path.c_str(), png.message);
    if (!png.width || !png.height || png.width > 1024 || png.height > 1024)
        throw Error("PNG '%s' exceeds 1024 dimensions", path.c_str());
    png.format = PNG_FORMAT_RGBA;
    Image image{png.width, png.height, {}};
    image.rgba.resize(PNG_IMAGE_SIZE(png));
    if (!png_image_finish_read(&png, nullptr, image.rgba.data(), 0, nullptr))
        throw Error("PNG '%s': %s", path.c_str(), png.message);
    return image;
}
nlohmann::json AssetArchive::json(const std::string& path) const {
    const auto& bytes = file(path);
    if (bytes.size() > 1024 * 1024)
        throw Error("JSON '%s' exceeds 1 MiB", path.c_str());
    try {
        return nlohmann::json::parse(bytes.begin(), bytes.end(),
                                     [](int depth, nlohmann::json::parse_event_t, nlohmann::json&) {
                                         if (depth > 32)
                                             throw Error("JSON nesting exceeds 32 levels");
                                         return true;
                                     });
    } catch (const std::bad_alloc&) {
        throw;
    } catch (const std::exception& e) {
        throw Error("JSON '%s': %s", path.c_str(), e.what());
    }
}
} // namespace voxel

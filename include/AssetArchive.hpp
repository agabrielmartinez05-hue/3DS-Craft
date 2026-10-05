#pragma once
#include "Assets.hpp"
#include "FileSystem.hpp"
#include <map>
namespace voxel {
class AssetArchive {
  public:
    static constexpr std::size_t MaxBytes = 32 * 1024 * 1024;
    void preload(const std::string& root, const Progress& progress = {});
    const std::vector<std::uint8_t>& file(const std::string& path) const;
    bool contains(const std::string& path) const { return files_.count(path) != 0; }
    std::vector<std::string> list(const std::string& prefix) const;
    const auto& directories() const { return directories_; }
    std::size_t bytes() const { return bytes_; }
    std::size_t fileCount() const { return files_.size(); }
    Image png(const std::string& path) const;
    nlohmann::json json(const std::string& path) const;

  private:
    std::map<std::string, std::vector<std::uint8_t>> files_;
    std::vector<std::string> directories_;
    std::size_t bytes_{};
};
} // namespace voxel

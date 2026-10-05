#pragma once
#include "AssetArchive.hpp"
#include "Assets.hpp"
#include "FileSystem.hpp"
#include "Gameplay.hpp"
#include <functional>
#include <map>

namespace voxel {
struct TextureRegion {
    std::string file;
    unsigned x{}, y{}, width{}, height{};
    std::array<float, 4> uv{}; // left, bottom, right, top (PICA orientation)
    bool transparent{};
};
struct BlockDefinition {
    bool breakable = true;
    bool opensInventory{};
};
struct ItemDefinition {
    unsigned texture{};
    unsigned stackLimit = 64;
    std::string displayName;
};
using TextureConsumer = std::function<void(unsigned, const Image&)>;
class AssetCatalog {
  public:
    static constexpr unsigned MaxAtlasSize = 1024, MaxTextures = 1024;
    static constexpr std::size_t MaxTextureBytes = MaxAtlasSize * MaxAtlasSize * 4;
    // root is sdmc:/assets/minecraft; one completed atlas is passed to the GPU consumer.
    void load(const std::string& root, const Progress& progress, const TextureConsumer& consume);
    void load(const AssetArchive& archive, const std::string& root, const Progress& progress,
              const TextureConsumer& consume);
    const auto& regions() const { return regions_; }
    const auto& models() const { return models_; }
    const auto& documents() const { return documents_; }
    const BlockDefinition& block(Block type) const { return blocks_.at(unsigned(type)); }
    unsigned item(Item type) const { return items_.at(unsigned(type)).texture; }
    const ItemDefinition& itemDefinition(Item type) const { return items_.at(unsigned(type)); }
    std::size_t textureBytes() const { return bytes_; }
    unsigned sprite(const std::string& id) const;
    unsigned atlasSize() const { return atlasSize_; }

  private:
    std::map<std::string, unsigned> sprites_;
    std::vector<TextureRegion> regions_;
    std::map<std::string, nlohmann::json> documents_;
    BlockModels models_;
    std::array<BlockDefinition, BlockCount> blocks_{};
    std::array<ItemDefinition, ItemCount> items_{};
    std::size_t bytes_{};
    unsigned atlasSize_{};
};
Image extractRegion(const Image& source, const TextureRegion& region);
std::vector<std::uint8_t> tileRgba8(const Image& image);
} // namespace voxel

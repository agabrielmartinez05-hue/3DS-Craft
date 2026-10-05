#pragma once
#include "AssetCatalog.hpp"
#include "Gui.hpp"
#include <citro2d.h>
#include <memory>

namespace voxel {
class GpuAssets {
  public:
    GpuAssets() = default;
    ~GpuAssets();
    void load(const std::string& root, const Progress& progress);
    C3D_Tex* texture(unsigned id) const;
    C2D_Image icon(Item item) const;
    C2D_Image sprite(const std::string& name) const;
    C2D_Image gui(GuiSprite id) const { return sprite(GuiSprites.at(unsigned(id))); }
    const AssetArchive& archive() const { return archive_; }
    const AssetCatalog& catalog() const { return catalog_; }

  private:
    struct Texture {
        C3D_Tex gpu{};
        Tex3DS_SubTexture view{};
        bool initialized{};
        ~Texture() {
            if (initialized)
                C3D_TexDelete(&gpu);
        }
    };
    AssetArchive archive_;
    AssetCatalog catalog_;
    std::unique_ptr<Texture> atlas_;
    std::vector<Tex3DS_SubTexture> views_;
};
} // namespace voxel

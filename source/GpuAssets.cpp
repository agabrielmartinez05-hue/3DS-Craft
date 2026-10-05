#include "GpuAssets.hpp"
#include "Error.hpp"
#include <cstring>

namespace voxel {
GpuAssets::~GpuAssets() {
    // Texture destruction must follow all prior UI/world submissions using it.
    if (C3D_FrameBegin(0))
        C3D_FrameEnd(0);
}
void GpuAssets::load(const std::string& root, const Progress& progress) {
    archive_.preload(root, progress);
    catalog_.load(archive_, "minecraft", progress, [&](unsigned id, const Image& image) {
        auto data = tileRgba8(image);
        auto texture = std::make_unique<Texture>();
        if (!C3D_TexInitVRAM(&texture->gpu, image.width, image.height, GPU_RGBA8))
            throw Error("Out of VRAM uploading texture %u (%ux%u)", id, image.width, image.height);
        texture->initialized = true;
        struct Staging {
            void* data;
            ~Staging() {
                if (data)
                    linearFree(data);
            }
        } staging{linearAlloc(data.size())};
        if (!staging.data)
            throw Error("Out of linear RAM staging texture %u", id);
        std::memcpy(staging.data, data.data(), data.size());
        GSPGPU_FlushDataCache(staging.data, data.size());
        // Called outside a frame: C3D_TexUpload waits for the VRAM copy before
        // staging is freed. CPU writes to VRAM are deliberately avoided.
        C3D_TexUpload(&texture->gpu, staging.data);
        // Verify the GPU copy once at startup, before releasing upload memory.
        std::memset(staging.data, 0, data.size());
        GSPGPU_FlushDataCache(staging.data, data.size());
        C3D_SyncTextureCopy(static_cast<u32*>(texture->gpu.data), 0,
                            static_cast<u32*>(staging.data), 0, data.size(), 8);
        GSPGPU_InvalidateDataCache(staging.data, data.size());
        if (std::memcmp(staging.data, data.data(), data.size()) != 0)
            throw Error("VRAM texture upload verification failed (atlas %u)", id);
        C3D_TexSetFilter(&texture->gpu, GPU_NEAREST, GPU_NEAREST);
        C3D_TexSetWrap(&texture->gpu, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
        texture->view = {static_cast<u16>(image.width), static_cast<u16>(image.height), 0, 1, 1, 0};
        atlas_ = std::move(texture);
    });
    views_.clear();
    for (const auto& r : catalog_.regions())
        views_.push_back({static_cast<u16>(r.width), static_cast<u16>(r.height), r.uv[0], r.uv[3],
                          r.uv[2], r.uv[1]});
}
C2D_Image GpuAssets::sprite(const std::string& name) const {
    return {texture(0), &views_.at(catalog_.sprite(name))};
}

C3D_Tex* GpuAssets::texture(unsigned id) const {
    if (id != 0 || !atlas_)
        throw Error("Texture id %u is not loaded", id);
    return &atlas_->gpu;
}
C2D_Image GpuAssets::icon(Item item) const {
    if (item >= Item::Count)
        throw Error("Invalid item icon");
    return {texture(0), &views_.at(catalog_.item(item))};
}
} // namespace voxel

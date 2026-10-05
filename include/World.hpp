#pragma once
#include "BlockStore.hpp"
#include <3ds.h>
#include <map>
#include <memory>
#include <vector>

namespace voxel {
class TerrainWorker {
  public:
    explicit TerrainWorker(const Terrain& terrain, const BlockModels* models);
    ~TerrainWorker();
    TerrainWorker(const TerrainWorker&) = delete;
    TerrainWorker& operator=(const TerrainWorker&) = delete;
    bool idle();
    void request(int x, int z, std::uint64_t revision, std::vector<BlockEdit> edits);
    std::unique_ptr<Column> take();
    bool threaded() const { return thread_ != nullptr; }

  private:
    static void entry(void* self);
    void run();
    const Terrain& terrain_;
    const BlockModels* models_;
    Thread thread_{};
    LightLock lock_{};
    LightEvent wake_{};
    bool stop_{}, busy_{};
    int x_{}, z_{};
    std::uint64_t revision_{};
    std::vector<BlockEdit> edits_;
    std::unique_ptr<Column> result_;
    char failure_[768]{};
};
struct GpuMesh {
    Vertex* vertices{};
    int count{};
    std::vector<MeshRange> ranges;
    ~GpuMesh();
    GpuMesh() = default;
    GpuMesh(const GpuMesh&) = delete;
    GpuMesh& operator=(const GpuMesh&) = delete;
};
class World {
  public:
    explicit World(std::uint32_t seed, unsigned version = 2, const BlockModels* models = nullptr,
                   Dimension dimension = Dimension::Overworld, WorldType type = WorldType::Normal)
        : blocks(seed, version, dimension, type), models_(models),
          worker_(blocks.terrain(), models) {}
    ~World();
    // Call after C3D_FrameBegin has waited for the previous GPU queue.
    void prime(Vec3 player);
    void releaseMeshes();
    void update(Vec3 player);
    unsigned vertexCount() const;
    const auto& chunks() const { return chunks_; }
    bool loaded(int columnX, int columnZ) const;
    bool threaded() const { return worker_.threaded(); }
    std::size_t meshBytes() const { return meshBytes_; }
    BlockStore blocks;

  private:
    void replace(SubChunk chunk);
    const BlockModels* models_; // Immutable catalog outlives World and its worker.
    TerrainWorker worker_;
    // Empty sub-chunks have no object, map entry, VBO, block array, or frame-loop visit.
    std::map<ChunkKey, std::unique_ptr<GpuMesh>> chunks_;
    std::vector<BlockPos> loadedColumns_; // Horizontal streaming metadata only.
    std::size_t meshBytes_{};
};
} // namespace voxel

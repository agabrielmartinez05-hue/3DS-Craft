#pragma once
#include "Core.hpp"
#include <map>
#include <set>

namespace voxel {
// No sub-chunk allocations: immutable terrain plus only deviations from it.
// Main-thread owned. Workers receive immutable, bounded edit snapshots.
class BlockStore final : public BlockSource {
  public:
    static constexpr std::size_t MaxEdits = 8192;
    explicit BlockStore(std::uint32_t seed, unsigned version = 2,
                        Dimension dimension = Dimension::Overworld,
                        WorldType type = WorldType::Normal)
        : terrain_(seed, version, dimension, type) {}
    Block block(int x, int y, int z) const override;
    bool set(BlockPos position, Block value);
    static bool editable(BlockPos position);
    const Terrain& terrain() const { return terrain_; }
    std::uint64_t revision() const { return revision_; }
    const auto& edits() const { return edits_; }
    std::size_t editCount() const { return edits_.size(); }
    std::vector<BlockEdit> snapshot(int columnX, int columnZ) const;
    std::set<ChunkKey> takeDirty();

  private:
    Terrain terrain_;
    std::map<BlockPos, Block> edits_;
    std::set<ChunkKey> dirty_;
    std::uint64_t revision_{};
    struct SurfaceCache {
        int x{}, z{};
        std::array<Block, WorldHeight> blocks{};
        bool valid{};
    };
    mutable std::array<SurfaceCache, 64> surfaceCache_{};
};
} // namespace voxel

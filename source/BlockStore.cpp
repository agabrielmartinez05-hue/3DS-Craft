#include "BlockStore.hpp"
#include <cmath>

namespace voxel {
bool BlockStore::editable(BlockPos p) {
    return p.y >= 0 && p.y < WorldHeight && p.x >= -int(WorldLimit) && p.x < int(WorldLimit) &&
           p.z >= -int(WorldLimit) && p.z < int(WorldLimit);
}
Block BlockStore::block(int x, int y, int z) const {
    if (y < 0)
        return terrain_.dimension() == Dimension::End ? Block::Air : Block::Stone;
    if (y >= WorldHeight)
        return Block::Air;
    auto edit = edits_.find({x, y, z});
    if (edit != edits_.end())
        return edit->second;
    const auto hash = (std::uint32_t(x) * 73856093u ^ std::uint32_t(z) * 19349663u) & 63u;
    auto& cached = surfaceCache_[hash];
    if (!cached.valid || cached.x != x || cached.z != z) {
        cached.x = x;
        cached.z = z;
        cached.valid = true;
        terrain_.sampleColumn(x, z, cached.blocks);
    }
    return cached.blocks[y];
}
bool BlockStore::set(BlockPos p, Block value) {
    if (!editable(p) || value >= Block::Count || block(p.x, p.y, p.z) == value)
        return false;
    const Block original = terrain_.block(p.x, p.y, p.z);
    if (value == original)
        edits_.erase(p);
    else {
        if (edits_.find(p) == edits_.end() && edits_.size() >= MaxEdits)
            return false;
        edits_[p] = value;
    }
    ++revision_;
    const BlockPos adjacent[7] = {p,
                                  {p.x - 1, p.y, p.z},
                                  {p.x + 1, p.y, p.z},
                                  {p.x, p.y - 1, p.z},
                                  {p.x, p.y + 1, p.z},
                                  {p.x, p.y, p.z - 1},
                                  {p.x, p.y, p.z + 1}};
    for (const auto& q : adjacent)
        if (editable(q))
            dirty_.insert(chunkOf(q));
    return true;
}
std::vector<BlockEdit> BlockStore::snapshot(int cx, int cz) const {
    std::vector<BlockEdit> result;
    // The map orders by x first; visit only the column's x interval.
    auto it = edits_.lower_bound({cx * 16 - 1, 0, -int(WorldLimit)});
    for (; it != edits_.end() && it->first.x <= cx * 16 + 16; ++it) {
        if (it->first.z >= cz * 16 - 1 && it->first.z <= cz * 16 + 16)
            result.push_back({it->first, it->second});
    }
    return result;
}
std::set<ChunkKey> BlockStore::takeDirty() {
    std::set<ChunkKey> result;
    result.swap(dirty_);
    return result;
}
} // namespace voxel

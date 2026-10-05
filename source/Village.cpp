#include "Village.hpp"
#include <algorithm>
#include <cmath>
namespace voxel {
VillageSite villageSite(const Terrain& terrain, int cx, int cz) {
    if (terrain.version() < 4 || terrain.dimension() != Dimension::Overworld ||
        terrain.type() != WorldType::Normal)
        return {};
    unsigned hash = terrain.seed() ^ (unsigned(cx) * 0x9e3779b9u) ^ (unsigned(cz) * 0x85ebca6bu);
    hash ^= hash >> 16;
    hash *= 0x7feb352du;
    hash ^= hash >> 15;
    if ((cx || cz) && (hash & 3))
        return {};
    const int x = cx * 128 + 24 + int((hash >> 8) & 15), z = cz * 128 + 24 + int((hash >> 16) & 15);
    auto center = terrain.surface(x, z);
    int low = center.height, high = low;
    if (center.biome != Biome::Plains)
        return {};
    for (int dz : {-14, 14})
        for (int dx : {-14, 14}) {
            auto h = terrain.surface(x + dx, z + dz).height;
            low = std::min(low, h);
            high = std::max(high, h);
        }
    if (high - low > 7 || high > WorldHeight - 10)
        return {};
    return {x, center.height, z, true};
}
void stampVillageColumn(const Terrain& terrain, int x, int z, std::array<Block, WorldHeight>& out) {
    const auto site = villageSite(terrain, floorDiv(x, 128), floorDiv(z, 128));
    if (!site.valid)
        return;
    int dx = x - site.x, dz = z - site.z;
    if (std::abs(dx) > 15 || std::abs(dz) > 15)
        return;
    const auto ground = [&](Block floor) {
        for (int y = std::max(0, site.y - 8); y < site.y; ++y)
            out[y] = y == site.y - 1 ? floor : Block::Dirt;
        for (int y = site.y; y < std::min(WorldHeight, site.y + 12); ++y)
            out[y] = Block::Air;
    };
    if ((std::abs(dx) <= 1 || std::abs(dz) <= 1 || dx == -7 || dx == 7) && std::abs(dx) <= 14 &&
        std::abs(dz) <= 14)
        ground(Block::Path);
    // Four 7x7 houses. Floor, walls/windows, two-block doorway, stepped roof, chest.
    static constexpr int houseOffsets[4][2] = {{-10, -10}, {4, -10}, {-10, 4}, {4, 4}};
    for (const auto& offset : houseOffsets) {
        int hx = dx - offset[0], hz = dz - offset[1];
        if (hx < 0 || hx > 6 || hz < 0 || hz > 6)
            continue;
        ground(Block::Cobblestone);
        for (int h = 0; h < 4; ++h) {
            bool edge = hx == 0 || hx == 6 || hz == 0 || hz == 6;
            bool door = hz == 6 && hx == 3 && h < 2;
            bool window = h == 2 && ((hx == 0 || hx == 6) && hz == 3);
            if (edge && !door && !window)
                out[site.y + h] =
                    ((hx == 0 || hx == 6) && (hz == 0 || hz == 6)) ? Block::OakLog : Block::Planks;
        }
        int roof = 4 + std::min({hx, 6 - hx, 2});
        out[site.y + roof] = Block::Planks;
        if (hx == 1 && hz == 1)
            out[site.y] = Block::Chest;
        return;
    }
    // Well template: cobblestone basin, open shaft and four roof-support posts.
    if (std::abs(dx) <= 2 && std::abs(dz) <= 2) {
        ground(Block::Cobblestone);
        bool rim = std::abs(dx) == 2 || std::abs(dz) == 2;
        out[site.y] = rim ? Block::Cobblestone : Block::Air;
        if (!rim) {
            out[site.y - 1] = Block::Air;
            out[site.y - 2] = Block::Cobblestone;
        }
        if (std::abs(dx) == 2 && std::abs(dz) == 2)
            for (int h = 1; h < 4; ++h)
                out[site.y + h] = Block::OakLog;
        out[site.y + 4] = Block::Cobblestone;
    }
}
} // namespace voxel

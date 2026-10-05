#pragma once
#include "Core.hpp"
namespace voxel {
// Rotation-independent voxel templates are evaluated in global coordinates.
struct VillageSite {
    int x{}, y{}, z{};
    bool valid{};
};
VillageSite villageSite(const Terrain& terrain, int cellX, int cellZ);
void stampVillageColumn(const Terrain& terrain, int x, int z,
                        std::array<Block, WorldHeight>& column);
} // namespace voxel

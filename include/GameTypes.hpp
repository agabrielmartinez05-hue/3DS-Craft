#pragma once
namespace voxel {
enum class GameMode : unsigned { Survival, Creative };
enum class WorldType : unsigned { Normal, Superflat };
enum class Dimension : unsigned { Overworld, Nether, End };
inline const char* dimensionName(Dimension d) {
    return d == Dimension::Nether ? "Nether" : d == Dimension::End ? "The End" : "Overworld";
}
} // namespace voxel

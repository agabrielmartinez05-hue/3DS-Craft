#pragma once
#include "Crafting.hpp"
#include "FileSystem.hpp"
#include "Gameplay.hpp"
#include "Survival.hpp"

namespace voxel {
struct WorldSave {
    std::uint32_t seed{};
    unsigned generator = 5;
    GameMode mode = GameMode::Survival;
    WorldType type = WorldType::Normal;
    Dimension dimension = Dimension::Overworld;
    Survival survival;
    std::array<std::map<BlockPos, std::array<unsigned, ItemCount>>, 3> chests;
    std::array<std::map<BlockPos, Furnace>, 3> furnaces;
    std::array<std::vector<BlockEdit>, 3> journals;
    std::array<Vec3, 3> arrivals{};
    std::array<bool, 3> visited{{true, false, false}};
    std::array<unsigned, ItemCount> dropped{};
    Vec3 dropPosition{};
    Dimension dropDimension = Dimension::Overworld;
    bool dropActive{};
    std::uint64_t generation{};
    std::string name;
    Vec3 feet{};
    float yaw{}, pitch{};
    bool crouched{};
    bool recovered{}; // A corrupt slot was skipped in favor of a valid generation.
    double daySeconds = 300;
    std::array<unsigned, ItemCount> inventory{{16, 16, 16}};
    unsigned selected{};
    std::vector<BlockEdit> edits;
};
struct WorldSummary {
    std::string id, name, error;
    std::uint32_t seed{};
    bool valid{};
};
class WorldStore {
  public:
    explicit WorldStore(std::string root) : root_(std::move(root)) {}
    std::vector<WorldSummary> list(const Progress& progress = {}) const;
    std::string create(const std::string& name, std::uint32_t seed, std::uint64_t nonce,
                       GameMode mode = GameMode::Survival, WorldType type = WorldType::Normal);
    WorldSave load(const std::string& id) const;
    void save(const std::string& id, const WorldSave& world) const;

  private:
    struct Latest {
        WorldSave save;
        int slot = -1;
        bool anyFile{};
    };
    Latest latest(const std::string& id) const;
    std::string directory(const std::string& id) const;
    std::string root_;
};
std::uint32_t parseSeed(const std::string& text);
} // namespace voxel

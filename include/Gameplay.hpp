#pragma once
#include "BlockStore.hpp"
#include <array>

namespace voxel {

class Inventory {
  public:
    static constexpr unsigned StackLimit = 999;
    unsigned count(Item item) const;
    bool add(Item item, unsigned amount = 1);
    bool remove(Item item, unsigned amount = 1);
    void select(int direction);
    bool choose(Item item);
    Item selected() const { return selected_; }
    Block selectedBlock() const;
    static Item fromBlock(Block block);
    static const char* name(Item item);

  private:
    std::array<unsigned, static_cast<unsigned>(Item::Count)> items_{};
    Item selected_ = Item::Grass;
};
bool transferChest(std::array<unsigned, ItemCount>& chest, Inventory& inventory, Item item,
                   bool deposit, bool wholeStack);
// Each inventory pane displays 27 aggregate item types per page.
constexpr unsigned InventoryPageSize = 27;
constexpr unsigned InventoryPages = (ItemCount + InventoryPageSize - 1) / InventoryPageSize;
inline Item inventoryPageItem(unsigned page, unsigned cell) {
    const unsigned index = page * InventoryPageSize + cell % InventoryPageSize;
    return page < InventoryPages && index < ItemCount ? static_cast<Item>(index) : Item::Count;
}
struct RayHit {
    bool hit{}, hasAdjacent{};
    BlockPos position{}, adjacent{};
    Block block = Block::Air;
    float distance{};
};
RayHit raycast(const BlockSource& world, Vec3 origin, Vec3 direction, float reach = 5.0f);
float rayBox(Vec3 origin, Vec3 direction, Aabb box, float reach);
const char* breakBlock(BlockStore& world, Inventory& inventory, const RayHit& hit);
const char* placeBlock(BlockStore& world, Inventory& inventory, const RayHit& hit,
                       const Aabb* blockers, std::size_t blockerCount);

struct DayLight {
    Vec3 sky, sunDirection;
    float ambient{}, diffuse{};
};
class DayNight {
  public:
    static constexpr double DaySeconds = 1200.0; // 24,000 ticks at 20 ticks/second.
    void advance(double elapsed);
    void setSeconds(double seconds);
    double seconds() const { return seconds_; }
    DayLight light() const;

  private:
    double seconds_ = 300.0; // Start at noon.
};
class FpsCounter {
  public:
    void frame(double elapsed);
    float fps() const { return fps_; }

  private:
    double elapsed_{};
    unsigned frames_{};
    float fps_{};
};
} // namespace voxel

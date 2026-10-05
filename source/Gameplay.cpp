#include "Gameplay.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace voxel {
unsigned Inventory::count(Item item) const {
    return item < Item::Count ? items_[static_cast<unsigned>(item)] : 0;
}
bool Inventory::add(Item item, unsigned amount) {
    if (item >= Item::Count || amount > StackLimit - count(item))
        return false;
    items_[static_cast<unsigned>(item)] += amount;
    return true;
}
bool Inventory::remove(Item item, unsigned amount) {
    if (item >= Item::Count || amount > count(item))
        return false;
    items_[static_cast<unsigned>(item)] -= amount;
    return true;
}
void Inventory::select(int direction) {
    auto it = std::find(HotbarItems.begin(), HotbarItems.end(), selected_);
    const int index = int(it - HotbarItems.begin());
    selected_ = HotbarItems[(index + int(HotbarItems.size()) + (direction < 0 ? -1 : 1)) %
                            HotbarItems.size()];
}
bool Inventory::choose(Item item) {
    if (item >= Item::Count)
        return false;
    selected_ = item;
    return true;
}
Block Inventory::selectedBlock() const {
    return ItemTypes[unsigned(selected_)].places;
}
Item Inventory::fromBlock(Block block) {
    return block < Block::Count ? BlockTypes[unsigned(block)].drop : Item::Count;
}
const char* Inventory::name(Item item) {
    return item < Item::Count ? ItemTypes[unsigned(item)].name : "None";
}
bool transferChest(std::array<unsigned, ItemCount>& chest, Inventory& inventory, Item item,
                   bool deposit, bool wholeStack) {
    if (item >= Item::Count || chest[unsigned(item)] > Inventory::StackLimit)
        return false;
    auto& count = chest[unsigned(item)];
    unsigned available = deposit ? inventory.count(item) : count;
    unsigned space = Inventory::StackLimit - (deposit ? count : inventory.count(item));
    unsigned amount = std::min({available, space, wholeStack ? Inventory::StackLimit : 1u});
    if (!amount)
        return false;
    if (deposit) {
        inventory.remove(item, amount);
        count += amount;
    } else {
        inventory.add(item, amount);
        count -= amount;
    }
    return true;
}
RayHit raycast(const BlockSource& world, Vec3 origin, Vec3 direction, float reach) {
    if (reach <= 0 || dot(direction, direction) < 0.000001f)
        return {};
    direction = normalized(direction);
    int cell[3] = {int(std::floor(origin.x)), int(std::floor(origin.y)), int(std::floor(origin.z))};
    const float p[3] = {origin.x, origin.y, origin.z},
                d[3] = {direction.x, direction.y, direction.z};
    int step[3]{};
    float next[3], delta[3];
    for (int i = 0; i < 3; ++i) {
        step[i] = d[i] > 0 ? 1 : -1;
        if (d[i] == 0)
            next[i] = delta[i] = std::numeric_limits<float>::infinity();
        else {
            delta[i] = std::abs(1 / d[i]);
            next[i] = (float(cell[i] + (step[i] > 0 ? 1 : 0)) - p[i]) / d[i];
        }
    }
    RayHit hit{};
    float distance = 0;
    // Reach is fixed to five blocks in gameplay; this bound also limits malformed callers.
    for (int iterations = 0; iterations < 256 && distance <= reach; ++iterations) {
        hit.position = {cell[0], cell[1], cell[2]};
        hit.block = world.block(cell[0], cell[1], cell[2]);
        if (hit.block != Block::Air) {
            hit.hit = true;
            hit.distance = distance;
            return hit;
        }
        hit.adjacent = hit.position;
        hit.hasAdjacent = true;
        int axis = next[0] <= next[1] && next[0] <= next[2] ? 0 : next[1] <= next[2] ? 1 : 2;
        distance = next[axis];
        next[axis] += delta[axis];
        cell[axis] += step[axis];
    }
    return {};
}
float rayBox(Vec3 origin, Vec3 direction, Aabb box, float reach) {
    float lo = 0, hi = reach;
    const float p[3] = {origin.x, origin.y, origin.z},
                d[3] = {direction.x, direction.y, direction.z};
    const float low[3] = {box.minimum.x, box.minimum.y, box.minimum.z},
                high[3] = {box.maximum.x, box.maximum.y, box.maximum.z};
    for (int i = 0; i < 3; ++i) {
        if (std::abs(d[i]) < 0.000001f) {
            if (p[i] < low[i] || p[i] > high[i])
                return -1;
            continue;
        }
        float a = (low[i] - p[i]) / d[i], b = (high[i] - p[i]) / d[i];
        if (a > b)
            std::swap(a, b);
        lo = std::max(lo, a);
        hi = std::min(hi, b);
        if (lo > hi)
            return -1;
    }
    return lo;
}
const char* breakBlock(BlockStore& world, Inventory& inventory, const RayHit& hit) {
    if (!hit.hit || !BlockStore::editable(hit.position))
        return "No block in reach";
    const auto p = hit.position;
    const Block block = world.block(p.x, p.y, p.z);
    if (block == Block::Air)
        return "Block already gone";
    const auto item = Inventory::fromBlock(block);
    if (inventory.count(item) == Inventory::StackLimit)
        return "Inventory stack full";
    if (!world.set(p, Block::Air))
        return "World edit limit reached";
    inventory.add(item);
    return "Block collected";
}
const char* placeBlock(BlockStore& world, Inventory& inventory, const RayHit& hit,
                       const Aabb* blockers, std::size_t blockerCount) {
    if (!hit.hit || !hit.hasAdjacent || !BlockStore::editable(hit.adjacent))
        return "No placement face";
    if (inventory.selectedBlock() == Block::Air)
        return "Selected item cannot be placed";
    const auto p = hit.adjacent;
    if (world.block(hit.position.x, hit.position.y, hit.position.z) == Block::Air)
        return "Target changed";
    if (world.block(p.x, p.y, p.z) != Block::Air)
        return "Space occupied";
    if (!inventory.count(inventory.selected()))
        return "Selected stack is empty";
    const Aabb cube{{float(p.x), float(p.y), float(p.z)},
                    {float(p.x + 1), float(p.y + 1), float(p.z + 1)}};
    for (std::size_t i = 0; i < blockerCount; ++i)
        if (cube.overlaps(blockers[i]))
            return "Player or animal in the way";
    if (!world.set(p, inventory.selectedBlock()))
        return "World edit limit reached";
    inventory.remove(inventory.selected());
    return "Block placed";
}
void DayNight::advance(double elapsed) {
    if (std::isfinite(elapsed) && elapsed > 0)
        seconds_ = std::fmod(seconds_ + elapsed, DaySeconds);
}
void DayNight::setSeconds(double seconds) {
    if (std::isfinite(seconds) && seconds >= 0 && seconds < DaySeconds)
        seconds_ = seconds;
}
DayLight DayNight::light() const {
    const float angle = float(seconds_ / DaySeconds) * 2 * Pi;
    const float elevation = std::sin(angle);
    float daylight = std::clamp((elevation + 0.15f) / 0.45f, 0.0f, 1.0f);
    daylight = daylight * daylight * (3 - 2 * daylight);
    const Vec3 night{0.025f, 0.035f, 0.09f}, day{0.50f, 0.74f, 0.92f};
    Vec3 sky = night * (1 - daylight) + day * daylight;
    const float twilight = std::max(0.0f, 1 - std::abs(elevation) / 0.25f) * 0.4f;
    sky = sky * (1 - twilight) + Vec3{0.85f, 0.33f, 0.16f} * twilight;
    return {sky, normalized({std::cos(angle), std::max(elevation, 0.05f), 0.25f}),
            0.12f + daylight * 0.20f, daylight * 0.68f};
}
void FpsCounter::frame(double elapsed) {
    if (!std::isfinite(elapsed) || elapsed <= 0)
        return;
    elapsed_ += elapsed;
    ++frames_;
    if (elapsed_ >= 0.5) {
        fps_ = float(frames_ / elapsed_);
        elapsed_ = 0;
        frames_ = 0;
    }
}
} // namespace voxel

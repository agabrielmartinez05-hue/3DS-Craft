#pragma once
#include "Gameplay.hpp"
#include <string>
namespace voxel {
struct Survival {
    float health = 20, hunger = 20, saturation = 5, exhaustion{}, attackAge = 1, shieldAge{},
          regenTime{}, starveTime{}, hurtTime{};
    Item offhand = Item::Count;
    bool elytra{}, gliding{}, blocking{};
    unsigned shieldDurability = 336, elytraDurability = 432;
    std::array<Item, 4> armor{{Item::Count, Item::Count, Item::Count, Item::Count}};
    std::array<unsigned, 4> armorWear{};
    std::array<unsigned, ItemCount> toolWear{};
    Vec3 glideVelocity{};
    float flightTime{}, fallTop{};
    BlockPos miningTarget{};
    Block miningBlock = Block::Air;
    float miningTime{};
    bool dead() const { return health <= 0; }
    void tick(float dt, GameMode mode, float distance);
    float charge(Item main) const;
    int attack(Item main, bool critical = false);
    bool equipArmor(Inventory& inventory, Item item);
    bool wearTool(Inventory& inventory, Item item, unsigned amount = 1);
    unsigned armorPoints() const;
    bool damage(float amount, bool blockable, bool inFront, GameMode mode);
    bool eat(Inventory& inventory, Item item);
    bool equipOffhand(Inventory& inventory, Item item);
    bool equipElytra(Inventory& inventory);
    void respawn();
    bool mine(const RayHit& hit, Item main, float dt, GameMode mode);
    bool glide(Player& player, const BlockSource& world, float dt, GameMode mode);
};
Vec3 safeArrival(const BlockSource& world, Dimension dimension, Vec3 preferred);
std::string randomWorldName(std::uint32_t& state);
} // namespace voxel

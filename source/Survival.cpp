#include "Survival.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace voxel {
void Survival::tick(float dt, GameMode mode, float distance) {
    dt = std::clamp(dt, 0.0f, 0.1f);
    attackAge += dt;
    hurtTime = std::max(0.0f, hurtTime - dt);
    shieldAge = blocking ? shieldAge + dt : 0;
    if (mode == GameMode::Creative) {
        health = 20;
        hunger = 20;
        return;
    }
    if (dead())
        return;
    exhaustion += distance * 0.01f;
    while (exhaustion >= 4) {
        exhaustion -= 4;
        if (saturation > 0)
            saturation = std::max(0.0f, saturation - 1);
        else
            hunger = std::max(0.0f, hunger - 1);
    }
    if (hunger >= 18 && health < 20) {
        regenTime += dt;
        if (regenTime >= 4) {
            regenTime -= 4;
            health = std::min(20.0f, health + 1);
            exhaustion += 3;
        }
    } else
        regenTime = 0;
    if (hunger == 0) {
        starveTime += dt;
        if (starveTime >= 4) {
            starveTime -= 4;
            damage(1, false, false, mode);
        }
    } else
        starveTime = 0;
}
float Survival::charge(Item main) const {
    const float speed = toolStats(main).attackSpeed;
    return std::clamp(attackAge * speed, 0.0f, 1.0f);
}
int Survival::attack(Item main, bool critical) {
    const float strength = charge(main);
    attackAge = 0;
    exhaustion += 0.1f;
    const float boost = critical && strength > .9f ? 1.5f : 1.f;
    return std::max(1, int(toolStats(main).damage * (0.2f + 0.8f * strength * strength) * boost));
}
unsigned Survival::armorPoints() const {
    unsigned sum = 0;
    for (auto item : armor)
        sum += armorStats(item).points;
    return sum;
}
bool Survival::wearTool(Inventory& inventory, Item item, unsigned amount) {
    const auto max = toolStats(item).durability;
    if (!max || !inventory.count(item))
        return false;
    auto& wear = toolWear[unsigned(item)];
    if (amount >= max - wear) {
        inventory.remove(item);
        wear = 0;
        return true;
    }
    wear += amount;
    return false;
}
bool Survival::equipArmor(Inventory& inventory, Item item) {
    auto stats = armorStats(item);
    if (stats.slot < 0 || (stats.slot == 1 && elytra))
        return false;
    Inventory next = inventory;
    const auto previous = armor[stats.slot];
    if (previous == item) {
        if (!next.add(item))
            return false;
        toolWear[unsigned(item)] = armorWear[stats.slot];
        armorWear[stats.slot] = 0;
        armor[stats.slot] = Item::Count;
    } else {
        if (!next.remove(item) || (previous != Item::Count && !next.add(previous)))
            return false;
        if (previous != Item::Count)
            toolWear[unsigned(previous)] = armorWear[stats.slot];
        armor[stats.slot] = item;
        armorWear[stats.slot] = toolWear[unsigned(item)];
    }
    inventory = next;
    return true;
}
bool Survival::damage(float amount, bool blockable, bool inFront, GameMode mode) {
    if (mode == GameMode::Creative || dead() || hurtTime > 0 || amount <= 0)
        return false;
    if (blockable && inFront && blocking && offhand == Item::Shield && shieldAge >= 0.25f &&
        shieldDurability) {
        const unsigned wear = unsigned(std::ceil(amount)) + 1;
        shieldDurability = shieldDurability > wear ? shieldDurability - wear : 0;
        if (!shieldDurability)
            offhand = Item::Count;
        hurtTime = 0.3f;
        return false;
    }
    if (blockable) {
        unsigned toughness = 0;
        for (auto item : armor)
            toughness += armorStats(item).toughness;
        const float points = float(armorPoints());
        const float reduction =
            std::min(20.f, std::max(points / 5, points - amount / (2 + toughness / 4.f)));
        const unsigned wear = std::max(1u, unsigned(amount / 4));
        amount *= 1 - reduction / 25;
        for (unsigned i = 0; i < armor.size(); ++i) {
            if (armor[i] == Item::Count)
                continue;
            armorWear[i] += wear;
            if (armorWear[i] >= armorStats(armor[i]).durability) {
                toolWear[unsigned(armor[i])] = 0;
                armor[i] = Item::Count;
                armorWear[i] = 0;
            }
        }
    }
    health = std::max(0.0f, health - amount);
    hurtTime = 0.5f;
    gliding = gliding && !dead();
    return true;
}
bool Survival::eat(Inventory& inventory, Item item) {
    const auto food = foodStats(item);
    if (!food.hunger || hunger >= 20 || !inventory.remove(item))
        return false;
    hunger = std::min(20.0f, hunger + food.hunger);
    saturation = std::min(hunger, saturation + food.saturation);
    return true;
}
bool Survival::equipOffhand(Inventory& inventory, Item item) {
    Inventory next = inventory;
    if (item != Item::Count && !next.remove(item))
        return false;
    if (offhand != Item::Count && !next.add(offhand))
        return false;
    inventory = next;
    offhand = item;
    blocking = false;
    if (item == Item::Shield && !shieldDurability)
        shieldDurability = 336;
    return true;
}
bool Survival::equipElytra(Inventory& inventory) {
    if (elytra) {
        if (!inventory.add(Item::Elytra))
            return false;
        elytra = false;
        gliding = false;
        return true;
    }
    if (armor[1] != Item::Count || !inventory.remove(Item::Elytra))
        return false;
    elytra = true;
    return true;
}
void Survival::respawn() {
    *this = Survival{};
}
bool Survival::mine(const RayHit& hit, Item main, float dt, GameMode mode) {
    if (!hit.hit || hit.block == Block::Lava) {
        miningBlock = Block::Air;
        miningTime = 0;
        return false;
    }
    if (!(hit.position == miningTarget) || hit.block != miningBlock) {
        miningTime = 0;
        miningTarget = hit.position;
        miningBlock = hit.block;
    }
    const float hardness = BlockTypes[unsigned(hit.block)].hardness;
    const auto tool = toolStats(main);
    const float speed = tool.kind == preferredTool(hit.block) ? tool.speed : 1.f;
    const float time =
        mode == GameMode::Creative
            ? .12f
            : std::max(.05f, hardness * (canHarvest(hit.block, main) ? 1.5f : 5.f) / speed);
    miningTime += dt;
    if (miningTime < time)
        return false;
    miningTime = 0;
    return true;
}
bool Survival::glide(Player& player, const BlockSource& world, float dt, GameMode mode) {
    if (!gliding || !elytra || elytraDurability <= 1 || player.grounded) {
        gliding = false;
        return false;
    }
    const auto direction = player.camera.forward();
    const float horizontal = std::sqrt(direction.x * direction.x + direction.z * direction.z);
    const float lift = std::cos(player.camera.pitch) * std::cos(player.camera.pitch);
    const float ticks = dt * 20;
    // Original gliding-style lift/dive model; no post-1.9 rocket boosts.
    glideVelocity.y += (-1.6f + lift * 1.2f) * ticks;
    if (glideVelocity.y < 0 && horizontal > 0.01f) {
        const float gain = -glideVelocity.y * 0.1f * lift * ticks;
        glideVelocity.y += gain;
        glideVelocity.x += direction.x / horizontal * gain;
        glideVelocity.z += direction.z / horizontal * gain;
    }
    if (player.camera.pitch > 0 && horizontal > 0.01f) {
        const float speed =
            std::sqrt(glideVelocity.x * glideVelocity.x + glideVelocity.z * glideVelocity.z);
        const float gain = speed * std::sin(player.camera.pitch) * 0.04f * ticks;
        glideVelocity.y += gain * 3.2f;
        glideVelocity.x -= direction.x / horizontal * gain;
        glideVelocity.z -= direction.z / horizontal * gain;
    }
    const float speed =
        std::sqrt(glideVelocity.x * glideVelocity.x + glideVelocity.z * glideVelocity.z);
    if (horizontal > 0.01f) {
        glideVelocity.x += (direction.x / horizontal * speed - glideVelocity.x) * 0.1f * ticks;
        glideVelocity.z += (direction.z / horizontal * speed - glideVelocity.z) * 0.1f * ticks;
    }
    glideVelocity.x *= std::pow(0.99f, ticks);
    glideVelocity.z *= std::pow(0.99f, ticks);
    glideVelocity.y *= std::pow(0.98f, ticks);
    Vec3 delta = glideVelocity * dt;
    const int steps = std::max(
        1, int(std::ceil(std::max({std::abs(delta.x), std::abs(delta.y), std::abs(delta.z)}) /
                         0.15f)));
    delta = delta * (1.0f / steps);
    for (int n = 0; n < steps; ++n) {
        if (!collides(world, player, player.feet + delta))
            player.feet = player.feet + delta;
        else {
            damage(std::max(0.0f, speed * 0.5f - 3), false, false, mode);
            gliding = false;
            player.verticalSpeed = 0;
            break;
        }
    }
    player.camera.eye = player.feet + Vec3{0, 1.62f, 0};
    fallTop = player.feet.y;
    flightTime += dt;
    if (flightTime >= 1) {
        flightTime -= 1;
        if (mode != GameMode::Creative)
            --elytraDurability;
    }
    return true;
}
Vec3 safeArrival(const BlockSource& world, Dimension dimension, Vec3 preferred) {
    const int px = int(std::floor(std::clamp(preferred.x, -WorldLimit + 10, WorldLimit - 10)));
    const int pz = int(std::floor(std::clamp(preferred.z, -WorldLimit + 10, WorldLimit - 10)));
    PhysicsBody body;
    for (int radius = 0; radius <= 8; ++radius)
        for (int z = -radius; z <= radius; ++z)
            for (int x = -radius; x <= radius; ++x) {
                for (int k = 1; k < WorldHeight - 2; ++k) {
                    const int y = dimension == Dimension::Nether ? k : WorldHeight - 2 - k;
                    Vec3 pos{float(px + x) + 0.5f, float(y) + 0.02f, float(pz + z) + 0.5f};
                    const auto floor = world.block(px + x, y - 1, pz + z);
                    if (floor != Block::Air && floor != Block::Lava && !collides(world, body, pos))
                        return pos;
                }
            }
    throw std::runtime_error("No safe arrival surface in this dimension");
}
std::string randomWorldName(std::uint32_t& state) {
    const char* first[] = {"Brave",  "Quiet", "Golden",  "Hidden", "Misty",  "Ancient",
                           "Silver", "Wild",  "Gentle",  "Bright", "Frozen", "Lucky",
                           "Sunny",  "Calm",  "Distant", "Amber"};
    const char* second[] = {"Oak", "Birch", "River", "Cedar", "Wolf",  "Raven", "Stone", "Willow",
                            "Fox", "Pine",  "Moon",  "Maple", "Cloud", "Deer",  "Star",  "Meadow"};
    const char* third[] = {"Valley", "Haven",  "Ridge", "Grove", "Island", "Forest",
                           "Harbor", "Garden", "Hill",  "Coast", "Peak",   "Trail",
                           "Field",  "Shore",  "Falls", "Hollow"};
    const auto next = [&] {
        state = state * 1664525u + 1013904223u;
        return (state >> 16) & 15;
    };
    const auto a = next(), b = next(), c = next();
    return std::string(first[a]) + "-" + second[b] + "-" + third[c];
}
} // namespace voxel

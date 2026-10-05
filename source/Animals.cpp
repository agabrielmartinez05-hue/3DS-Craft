#include "Animals.hpp"
#include "Survival.hpp"
#include "Village.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace voxel {
namespace {
// Axis-aligned local boxes are rotated about the animal's feet. Six outward faces.
void box(std::vector<Vertex>& vertices, Vec3 low, Vec3 high, Vec3 color, Vec3 origin, float yaw) {
    const float cosine = std::cos(yaw), sine = std::sin(yaw);
    auto rotate = [&](Vec3 p) {
        return Vec3{cosine * p.x - sine * p.z, p.y, sine * p.x + cosine * p.z};
    };
    const float minimum[3] = {low.x, low.y, low.z}, maximum[3] = {high.x, high.y, high.z};
    for (int axis = 0; axis < 3; ++axis)
        for (int sign = -1; sign <= 1; sign += 2) {
            int u = (axis + 1) % 3, v = (axis + 2) % 3;
            Vec3 points[4];
            for (int corner = 0; corner < 4; ++corner) {
                float p[3]{};
                p[axis] = sign > 0 ? maximum[axis] : minimum[axis];
                p[u] = (corner == 1 || corner == 2) ? maximum[u] : minimum[u];
                p[v] = corner >= 2 ? maximum[v] : minimum[v];
                points[corner] = rotate({p[0], p[1], p[2]}) + origin;
            }
            float normal[3]{};
            normal[axis] = float(sign);
            const auto n = rotate({normal[0], normal[1], normal[2]});
            const int positive[6] = {0, 1, 2, 0, 2, 3}, negative[6] = {0, 2, 1, 0, 3, 2};
            const auto indices = sign > 0 ? positive : negative;
            for (int k = 0; k < 6; ++k) {
                auto p = points[indices[k]];
                vertices.push_back({p.x, p.y, p.z, color.x, color.y, color.z, n.x, n.y, n.z});
            }
        }
}
} // namespace
GeneralAnimal::GeneralAnimal(Species species, Vec3 feet, std::uint32_t seed, int hp, float speed,
                             Vec3 color)
    : species_(species), speed_(speed), color_(color) {
    body.feet = feet;
    body.radius = 0.75f;
    body.height = 1.2f;
    health = {hp, hp};
    wander.random = seed;
    wander.remaining = 1.0f + randomUnit() * 2.0f;
}
float GeneralAnimal::randomUnit() {
    wander.random = wander.random * 1664525u + 1013904223u;
    return float(wander.random >> 8) * (1.0f / 16777216.0f);
}
void GeneralAnimal::update(const BlockSource& blocks, float dt) {
    if (health.current <= 0)
        return;
    dt = std::clamp(dt, 0.0f, 0.05f);
    wander.remaining -= dt;
    if (wander.remaining <= 0) {
        if (wander.state == WanderComponent::State::Idle) {
            wander.state = WanderComponent::State::Wandering;
            transform.yaw = randomUnit() * 2 * Pi;
            wander.remaining = 2.0f + randomUnit() * 3.0f;
        } else {
            wander.state = WanderComponent::State::Idle;
            wander.remaining = idleSeconds() + randomUnit();
        }
    }
    Movement move{};
    if (wander.state == WanderComponent::State::Wandering) {
        move.forward = 1;
        const Vec3 ahead =
            body.feet + Vec3{std::sin(transform.yaw), 0, -std::cos(transform.yaw)} * 0.9f;
        const int x = int(std::floor(ahead.x)), z = int(std::floor(ahead.z)),
                  y = int(std::floor(body.feet.y));
        const bool cliff =
            blocks.block(x, y - 1, z) == Block::Air && blocks.block(x, y - 2, z) == Block::Air;
        if (cliff) {
            move.forward = 0;
            transform.yaw += Pi * 0.5f;
        } else if (collides(blocks, body, ahead)) {
            if (body.grounded && !collides(blocks, body, ahead + Vec3{0, 1.05f, 0}))
                move.jump = true;
            else {
                move.forward = 0;
                transform.yaw += Pi * 0.5f;
            }
        }
    }
    moveBody(blocks, body, move, transform.yaw, speed_, dt);
}
bool GeneralAnimal::damage(int amount) {
    health.current = std::max(0, health.current - std::max(0, amount));
    return health.current == 0;
}
void GeneralAnimal::appendModel(std::vector<Vertex>& vertices) const {
    const auto add = [&](Vec3 low, Vec3 high, Vec3 color) {
        box(vertices, low, high, color, body.feet, transform.yaw);
    };
    add({-0.38f, 0.35f, -0.4f}, {0.38f, 0.95f, 0.4f}, color_);
    add({-0.24f, 0.64f, -0.63f}, {0.24f, 1.12f, -0.28f}, color_);
    const Vec3 legs = species_ == Species::Sheep ? Vec3{0.36f, 0.28f, 0.20f} : color_ * 0.65f;
    for (int x = -1; x <= 1; x += 2)
        for (int z = -1; z <= 1; z += 2) {
            Vec3 low{float(x) * 0.25f - 0.08f, 0, float(z) * 0.26f - 0.08f};
            add(low, low + Vec3{0.16f, 0.40f, 0.16f}, legs);
        }
    if (species_ == Species::Cow) {
        add({-0.39f, 0.53f, -0.15f}, {-0.37f, 0.83f, 0.22f}, {0.95f, 0.94f, 0.90f});
        add({0.37f, 0.42f, -0.30f}, {0.39f, 0.73f, 0.06f}, {0.95f, 0.94f, 0.90f});
    } else if (species_ == Species::Pig)
        add({-0.15f, 0.71f, -0.67f}, {0.15f, 0.9f, -0.62f}, {0.74f, 0.26f, 0.36f});
    else
        add({-0.16f, 0.72f, -0.65f}, {0.16f, 0.99f, -0.62f}, {0.4f, 0.32f, 0.26f});
}
SheepAnimal::SheepAnimal(Vec3 feet, std::uint32_t seed)
    : GeneralAnimal(Species::Sheep, feet, seed, AnimalDefinitions[0].health,
                    AnimalDefinitions[0].speed, {0.95f, 0.94f, 0.89f}) {}
PigAnimal::PigAnimal(Vec3 feet, std::uint32_t seed)
    : GeneralAnimal(Species::Pig, feet, seed, AnimalDefinitions[1].health,
                    AnimalDefinitions[1].speed, {0.96f, 0.53f, 0.64f}) {}
CowAnimal::CowAnimal(Vec3 feet, std::uint32_t seed)
    : GeneralAnimal(Species::Cow, feet, seed, AnimalDefinitions[2].health,
                    AnimalDefinitions[2].speed, {0.35f, 0.19f, 0.10f}) {}
void AnimalSystem::spawn(const BlockStore& blocks) {
    animals_.clear();
    animals_.reserve(MaxAnimals);
    for (int i = 0; i < 3; ++i) {
        const int x = (i - 1) * 4, z = -5;
        // Use the highest supporting voxel across the whole body footprint.
        int height = 0;
        for (int dx = -1; dx <= 1; ++dx)
            for (int dz = -1; dz <= 1; ++dz)
                for (int y = WorldHeight - 1; y >= 0; --y) {
                    if (blocks.block(x + dx, y, z + dz) != Block::Air) {
                        height = std::max(height, y + 1);
                        break;
                    }
                }
        const Vec3 p{float(x) + 0.5f, float(height) + 0.05f, float(z) + 0.5f};
        if (i == 0)
            animals_.push_back(std::make_unique<SheepAnimal>(p, 101));
        if (i == 1)
            animals_.push_back(std::make_unique<PigAnimal>(p, 202));
        if (i == 2)
            animals_.push_back(std::make_unique<CowAnimal>(p, 303));
    }
}
void AnimalSystem::update(const BlockSource& blocks, Vec3 player, float dt) {
    for (auto& animal : animals_) {
        if (animal->species() == Species::Zombie)
            static_cast<ZombieAnimal*>(animal.get())->target = player;
        const auto difference = animal->body.feet - player;
        if (dot(difference, difference) <= 24 * 24)
            animal->update(blocks, dt);
    }
}
const char* AnimalSystem::attack(const BlockSource& blocks, Inventory& inventory,
                                 const Camera& camera, int amount, bool sweep) {
    float closest = 4.0f;
    auto target = animals_.end();
    const auto wall = raycast(blocks, camera.eye, camera.forward(), closest);
    if (wall.hit)
        closest = wall.distance;
    for (auto it = animals_.begin(); it != animals_.end(); ++it) {
        const float distance = rayBox(camera.eye, camera.forward(), (*it)->bounds(), closest);
        if (distance >= 0 && distance < closest) {
            closest = distance;
            target = it;
        }
    }
    if (target == animals_.end())
        return "No animal in reach";
    const auto primary = target->get();
    std::array<int, MaxAnimals> damage{};
    Inventory next = inventory;
    bool killed = false;
    for (unsigned i = 0; i < animals_.size(); ++i) {
        auto& animal = animals_[i];
        if (animal.get() == primary)
            damage[i] = amount;
        else if (sweep) {
            const auto offset = animal->body.feet - primary->body.feet;
            auto center = animal->body.feet + Vec3{0, animal->body.height * .5f, 0};
            auto fromPlayer = center - camera.eye;
            const float reach = std::sqrt(dot(fromPlayer, fromPlayer));
            if (dot(offset, offset) <= 2.25f && reach < 4 &&
                !raycast(blocks, camera.eye, fromPlayer, reach).hit)
                damage[i] = 1;
        }
        if (damage[i] && animal->health.current <= damage[i]) {
            const auto drop = animal->drop();
            if (drop.count && !next.add(drop.item, drop.count))
                return "Drop stack full";
        }
    }
    for (unsigned i = unsigned(animals_.size()); i-- > 0;)
        if (damage[i] && animals_[i]->damage(damage[i])) {
            animals_.erase(animals_.begin() + i);
            killed = true;
        }
    inventory = next;
    return killed ? "Animal drop collected" : sweep ? "Sweeping hit" : "Animal hit";
}
void ZombieAnimal::update(const BlockSource& blocks, float dt) {
    const auto d = target - body.feet;
    transform.yaw = std::atan2(d.x, -d.z);
    if (dot(d, d) > 1.2f)
        moveBody(blocks, body, {1, 0, body.grounded}, transform.yaw, 1.2f, dt);
}
void AnimalSystem::spawnHostile(const BlockSource& blocks, Vec3 player, Dimension dimension) {
    if (animals_.size() >= MaxAnimals)
        return;
    for (const auto& a : animals_)
        if (a->species() == Species::Zombie)
            return;
    try {
        animals_.push_back(std::make_unique<ZombieAnimal>(
            safeArrival(blocks, dimension, player + Vec3{8, 0, -8})));
    } catch (const std::runtime_error&) {
        return;
    }
}
bool AnimalSystem::threat(Vec3 player, Vec3& source) const {
    for (const auto& a : animals_)
        if (a->species() == Species::Zombie) {
            const auto d = a->body.feet - player;
            if (dot(d, d) < 2.6f) {
                source = a->body.feet;
                return true;
            }
        }
    return false;
}
void AnimalSystem::appendVisible(const Camera& camera, std::vector<Vertex>& vertices,
                                 bool villagers) const {
    vertices.clear();
    for (const auto& animal : animals_) {
        if ((animal->species() == Species::Villager) != villagers)
            continue;
        const auto b = animal->bounds();
        if (camera.visible(b.minimum, b.maximum))
            animal->appendModel(vertices);
    }
}
void VillagerAnimal::appendModel(std::vector<Vertex>& vertices) const {
    const auto add = [&](Vec3 lo, Vec3 hi, unsigned u, unsigned v, unsigned w, unsigned h,
                         unsigned d) {
        const auto first = vertices.size();
        box(vertices, lo, hi, {1, 1, 1}, body.feet, transform.yaw);
        const unsigned uv[6][4] = {{u, v + d, d, h},     {u + d + w, v + d, d, h},
                                   {u + d + w, v, w, d}, {u + d, v, w, d},
                                   {u + d, v + d, w, h}, {u + 2 * d + w, v + d, w, h}};
        for (unsigned face = 0; face < 6; ++face)
            for (unsigned n = 0; n < 6; ++n) {
                auto& vertex = vertices[first + face * 6 + n];
                const unsigned order[2][6] = {{0, 2, 1, 0, 3, 2}, {0, 1, 2, 0, 2, 3}};
                auto corner = order[face % 2][n];
                float x = uv[face][0] + ((corner == 1 || corner == 2) ? uv[face][2] : 0),
                      y = uv[face][1] + (corner >= 2 ? 0 : uv[face][3]);
                vertex.u = skin[0] + (skin[2] - skin[0]) * x / 64;
                vertex.v = skin[3] - (skin[3] - skin[1]) * y / 64;
            }
    };
    add({-.25f, 1.3f, -.25f}, {.25f, 1.925f, .25f}, 0, 0, 8, 10, 8);
    add({-.063f, 1.32f, -.375f}, {.063f, 1.57f, -.25f}, 24, 0, 2, 4, 2);
    add({-.25f, .55f, -.1875f}, {.25f, 1.3f, .1875f}, 16, 20, 8, 12, 6);
    add({-.25f, .85f, -.35f}, {.25f, 1.10f, -.10f}, 40, 22, 8, 4, 4);
    add({-.25f, 0, -.125f}, {0, .55f, .125f}, 0, 22, 4, 12, 4);
    add({0, 0, -.125f}, {.25f, .55f, .125f}, 0, 22, 4, 12, 4);
}
void AnimalSystem::updateVillages(const BlockStore& blocks, Vec3 player, float dt,
                                  std::array<float, 4> skin) {
    villageTimer_ += dt;
    if (villageTimer_ < 1)
        return;
    villageTimer_ = 0;
    VillageSite nearest;
    float best = 48 * 48;
    int cx = floorDiv(int(std::floor(player.x)), 128),
        cz = floorDiv(int(std::floor(player.z)), 128);
    for (int z = cz - 1; z <= cz + 1; ++z)
        for (int x = cx - 1; x <= cx + 1; ++x) {
            auto site = villageSite(blocks.terrain(), x, z);
            float d = (site.x - player.x) * (site.x - player.x) +
                      (site.z - player.z) * (site.z - player.z);
            if (site.valid && d < best) {
                nearest = site;
                best = d;
            }
        }
    if (!nearest.valid) {
        animals_.erase(
            std::remove_if(animals_.begin(), animals_.end(),
                           [](const auto& a) { return a->species() == Species::Villager; }),
            animals_.end());
        villageX_ = villageZ_ = 99999;
        return;
    }
    if (villageX_ == nearest.x && villageZ_ == nearest.z)
        return;
    animals_.erase(std::remove_if(animals_.begin(), animals_.end(),
                                  [](const auto& a) { return a->species() == Species::Villager; }),
                   animals_.end());
    villageX_ = nearest.x;
    villageZ_ = nearest.z;
    for (int i = 0; i < 3 && animals_.size() < MaxAnimals; ++i) {
        Vec3 feet{float(nearest.x + 4 + i * 2) + .5f, float(nearest.y) + .02f,
                  float(nearest.z) + .5f};
        auto villager = std::make_unique<VillagerAnimal>(feet, unsigned(nearest.x + nearest.z + i));
        if (collides(blocks, villager->body, feet))
            continue;
        villager->skin = skin;
        animals_.push_back(std::move(villager));
    }
}
} // namespace voxel

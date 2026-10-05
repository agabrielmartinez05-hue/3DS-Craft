#pragma once
#include "Gameplay.hpp"
#include <memory>

namespace voxel {
enum class Species { Sheep, Pig, Cow, Zombie, Villager };
struct TransformComponent {
    float yaw{};
};
struct HealthComponent {
    int current{}, maximum{};
};
struct WanderComponent {
    enum class State { Idle, Wandering };
    State state = State::Idle;
    float remaining = 1.0f;
    std::uint32_t random = 1;
};
struct AnimalDrop {
    Item item;
    unsigned count;
};

class GeneralAnimal {
  public:
    virtual ~GeneralAnimal() = default;
    virtual void update(const BlockSource& blocks, float seconds);
    bool damage(int amount);
    Aabb bounds() const { return body.bounds(); }
    virtual void appendModel(std::vector<Vertex>& vertices) const;
    virtual AnimalDrop drop() const = 0;
    Species species() const { return species_; }
    PhysicsBody body;
    TransformComponent transform;
    HealthComponent health;
    WanderComponent wander;

  protected:
    GeneralAnimal(Species species, Vec3 feet, std::uint32_t seed, int health, float speed,
                  Vec3 color);
    virtual float idleSeconds() const = 0;

  private:
    float randomUnit();
    Species species_;
    float speed_;
    Vec3 color_;
};
class SheepAnimal final : public GeneralAnimal {
  public:
    SheepAnimal(Vec3 feet, std::uint32_t seed);
    AnimalDrop drop() const override {
        return {AnimalDefinitions[0].drop, AnimalDefinitions[0].dropCount};
    }

  private:
    float idleSeconds() const override { return 4.0f; } // Grazing pauses.
};
class PigAnimal final : public GeneralAnimal {
  public:
    PigAnimal(Vec3 feet, std::uint32_t seed);
    AnimalDrop drop() const override {
        return {AnimalDefinitions[1].drop, AnimalDefinitions[1].dropCount};
    }

  private:
    float idleSeconds() const override { return 1.2f; }
};
class CowAnimal final : public GeneralAnimal {
  public:
    CowAnimal(Vec3 feet, std::uint32_t seed);
    AnimalDrop drop() const override {
        return {AnimalDefinitions[2].drop, AnimalDefinitions[2].dropCount};
    }

  private:
    float idleSeconds() const override { return 3.0f; }
};
class ZombieAnimal final : public GeneralAnimal {
  public:
    ZombieAnimal(Vec3 feet)
        : GeneralAnimal(Species::Zombie, feet, 401, 20, 1.2f, {0.2f, 0.55f, 0.18f}) {}
    Vec3 target{};
    void update(const BlockSource& blocks, float dt) override;
    AnimalDrop drop() const override { return {Item::IronIngot, 1}; }

  private:
    float idleSeconds() const override { return 0; }
};
class VillagerAnimal final : public GeneralAnimal {
  public:
    VillagerAnimal(Vec3 feet, std::uint32_t seed)
        : GeneralAnimal(Species::Villager, feet, seed, 20, 0.7f, {1, 1, 1}) {
        body.radius = .4f;
        body.height = 1.95f;
    }
    std::array<float, 4> skin{{0, 0, 1, 1}};
    void appendModel(std::vector<Vertex>& vertices) const override;
    AnimalDrop drop() const override { return {Item::Count, 0}; }

  private:
    float idleSeconds() const override { return 3; }
};
class AnimalSystem {
  public:
    static constexpr std::size_t MaxAnimals = 12;
    static constexpr std::size_t MaxModelVertices = MaxAnimals * 8 * 36;
    void spawn(const BlockStore& blocks);
    void updateVillages(const BlockStore& blocks, Vec3 player, float dt, std::array<float, 4> skin);
    void spawnHostile(const BlockSource& blocks, Vec3 player, Dimension dimension);
    bool threat(Vec3 player, Vec3& source) const;
    void update(const BlockSource& blocks, Vec3 player, float seconds);
    const char* attack(const BlockSource& blocks, Inventory& inventory, const Camera& camera,
                       int damage = 4, bool sweep = false);
    void appendVisible(const Camera& camera, std::vector<Vertex>& vertices,
                       bool villagers = false) const;
    const auto& animals() const { return animals_; }

  private:
    std::vector<std::unique_ptr<GeneralAnimal>> animals_;
    float villageTimer_ = 1;
    int villageX_ = 99999, villageZ_ = 99999;
};
} // namespace voxel

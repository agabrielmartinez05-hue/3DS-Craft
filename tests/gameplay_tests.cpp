#include "Animals.hpp"
#include "Crafting.hpp"
#include "Survival.hpp"
#include "Village.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

using namespace voxel;
namespace {
struct FlatWorld final : BlockSource {
    Block block(int, int y, int) const override { return y < 0 ? Block::Stone : Block::Air; }
};
void testEditsAndRays() {
    BlockStore world(1337);
    const auto original = world.block(-1, 20, -1);
    assert(world.set({-1, 20, -1}, Block::Air));
    assert(world.block(-1, 20, -1) == Block::Air);
    assert(world.set({-1, 20, -1}, original));
    assert(world.editCount() == 0);
    world.takeDirty();
    assert(world.set({15, 47, 15}, Block::Grass));
    auto dirty = world.takeDirty();
    assert(dirty.size() == 4);
    assert(dirty.count({0, 2, 0}) && dirty.count({1, 2, 0}) && dirty.count({0, 3, 0}) &&
           dirty.count({0, 2, 1}));
    assert(world.snapshot(1, 0).size() == 1);
    assert(world.snapshot(2, 0).empty());
    assert(!world.set({0, -1, 0}, Block::Air) && !world.set({0, 64, 0}, Block::Stone));
    assert(world.set({11, 55, 8}, Block::Stone));
    const auto ray = raycast(world, {8.5f, 55.5f, 8.5f}, {7, 0, 0});
    assert(ray.hit && ray.hasAdjacent && (ray.position == BlockPos{11, 55, 8}));
    assert((ray.adjacent == BlockPos{10, 55, 8}) && std::abs(ray.distance - 2.5f) < 0.001f);
    assert(!raycast(world, {8.5f, 55.5f, 8.5f}, {1, 0, 0}, 2).hit);
    assert(!raycast(world, {8.5f, 55.5f, 8.5f}, {}).hit);
    assert(world.set({-17, 55, 8}, Block::Stone));
    const auto negative = raycast(world, {-15.5f, 55.5f, 8.5f}, {-1, 0, 0});
    assert(negative.hit && (negative.position == BlockPos{-17, 55, 8}));
    assert(std::abs(negative.distance - 0.5f) < 0.001f);
    const auto inside = raycast(world, {-16.5f, 55.5f, 8.5f}, {0, 1, 0});
    assert(inside.hit && !inside.hasAdjacent && inside.distance == 0);
    assert(rayBox({0, 0, 0}, {0, 0, -1}, {{-1, -1, -4}, {1, 1, -2}}, 5) == 2);
    assert(rayBox({0, 0, 0}, {0, 1, 0}, {{-1, -1, -4}, {1, 1, -2}}, 5) < 0);

    Inventory inventory;
    assert(std::strcmp(breakBlock(world, inventory, ray), "Block collected") == 0);
    assert(inventory.count(Item::Cobblestone) == 1 && world.block(11, 55, 8) == Block::Air);
    inventory.select(1);
    inventory.select(1);
    assert(inventory.selected() == Item::Stone);
    inventory.choose(Item::Cobblestone);
    assert(world.set({12, 55, 8}, Block::Dirt));
    const auto place = raycast(world, {8.5f, 55.5f, 8.5f}, {1, 0, 0});
    const Aabb obstruction{{11, 55, 8}, {12, 57, 9}};
    assert(std::strcmp(placeBlock(world, inventory, place, &obstruction, 1),
                       "Player or animal in the way") == 0);
    assert(inventory.count(Item::Cobblestone) == 1 && world.block(11, 55, 8) == Block::Air);
    assert(std::strcmp(placeBlock(world, inventory, place, nullptr, 0), "Block placed") == 0);
    assert(world.block(11, 55, 8) == Block::Cobblestone && inventory.count(Item::Cobblestone) == 0);
    const auto next = raycast(world, {8.5f, 55.5f, 8.5f}, {1, 0, 0});
    assert(std::strcmp(placeBlock(world, inventory, next, nullptr, 0), "Selected stack is empty") ==
           0);
    assert(inventory.add(Item::Cobblestone, Inventory::StackLimit));
    assert(std::strcmp(breakBlock(world, inventory, next), "Inventory stack full") == 0);
    assert(world.block(11, 55, 8) == Block::Cobblestone);
    PhysicsBody body;
    body.feet = {11.5f, 55, 8.5f};
    assert(collides(world, body, body.feet));
    world.set({11, 55, 8}, Block::Air);
    assert(!collides(world, body, body.feet));
}
void testCrouch() {
    struct Ceiling final : BlockSource {
        Block block(int x, int y, int) const override {
            return y < 0 || (x == 0 && y == 2) ? Block::Stone : Block::Air;
        }
    } ceiling;
    Player player;
    player.feet = {0.5f, 0.5f, 0.5f};
    player.step(ceiling, {}, 0, true);
    assert(player.height == 1.3f && std::abs(player.camera.eye.y - 1.62f) < 0.001f);
    player.step(ceiling, {}, 0, false);
    assert(player.height == 1.3f); // Cannot stand into ceiling.
    player.feet.x = 2.5f;
    player.step(ceiling, {}, 0, false);
    assert(player.height == 1.8f);
    FlatWorld flat;
    Player walking, crouched;
    walking.feet = crouched.feet = {0, 0.02f, 0};
    for (int i = 0; i < 60; ++i) {
        walking.step(flat, {0, 1, false}, 1.0f / 60, false);
        crouched.step(flat, {0, 1, false}, 1.0f / 60, true);
    }
    assert(walking.feet.x > crouched.feet.x * 2);
}
void testLimits() {
    BlockStore world(1337);
    for (unsigned i = 0; i < BlockStore::MaxEdits; ++i)
        assert(world.set({int(i % 128), 60, int(i / 128)}, Block::Stone));
    assert(world.editCount() == BlockStore::MaxEdits);
    assert(!world.set({200, 60, 200}, Block::Stone));
    assert(world.set({0, 60, 0}, Block::Air));
    assert(world.set({200, 60, 200}, Block::Stone));
    assert(world.editCount() == BlockStore::MaxEdits);
}
void testTimeAndAnimals() {
    DayNight cycle;
    const auto noon = cycle.light();
    assert(noon.diffuse > 0.6f && noon.ambient > 0.3f);
    cycle.advance(600);
    const auto night = cycle.light();
    assert(night.diffuse < 0.01f && night.sky.y < noon.sky.y && night.ambient > 0);
    cycle.advance(1200 * 1000.0 + 600);
    assert(std::abs(cycle.seconds() - 300) < 0.00001);
    assert(std::abs(cycle.light().diffuse - noon.diffuse) < 0.00001f);
    cycle.advance(900);
    const auto dawn = cycle.light();
    cycle.advance(600);
    const auto dusk = cycle.light();
    assert(std::abs(dawn.ambient - dusk.ambient) < 0.0001f);
    assert(dot(dawn.sunDirection, dusk.sunDirection) < 0);
    FpsCounter fps;
    for (int i = 0; i < 120; ++i)
        fps.frame(1.0 / 60);
    assert(std::abs(fps.fps() - 60) < 0.01f);
    for (int i = 0; i < 60; ++i)
        fps.frame(1.0 / 30);
    assert(std::abs(fps.fps() - 30) < 0.01f);

    FlatWorld flat;
    SheepAnimal sheep({0, 0.02f, 0}, 12);
    PigAnimal pig({3, 0.02f, 0}, 13);
    CowAnimal cow({6, 0.02f, 0}, 14);
    assert(sheep.drop().item == Item::Wool && pig.drop().item == Item::Pork &&
           cow.drop().item == Item::Leather);
    for (int i = 0; i < 1200; ++i) {
        sheep.update(flat, 1.0f / 60);
        pig.update(flat, 1.0f / 60);
        cow.update(flat, 1.0f / 60);
    }
    assert(sheep.body.grounded && pig.body.grounded && cow.body.grounded);
    assert(dot(sheep.body.feet, sheep.body.feet) > 0.1f);
    assert(sheep.body.feet.y >= 0 && sheep.body.feet.y < 0.1f);
    struct Cliff final : BlockSource {
        Block block(int x, int y, int) const override {
            return x >= 0 && y < 0 ? Block::Stone : Block::Air;
        }
    } cliff;
    PigAnimal cautious({1, 0.02f, 0}, 10);
    cautious.transform.yaw = -Pi / 2;
    cautious.wander.state = WanderComponent::State::Wandering;
    cautious.wander.remaining = 10;
    for (int i = 0; i < 120; ++i)
        cautious.update(cliff, 1.0f / 60);
    assert(cautious.body.feet.x >= cautious.body.radius && cautious.body.feet.y >= 0);
    std::vector<Vertex> vertices;
    cow.transform.yaw = 0.73f;
    cow.appendModel(vertices);
    assert(vertices.size() == 8 * 36);
    const auto bounds = cow.bounds();
    for (std::size_t i = 0; i < vertices.size(); i += 3) {
        const auto& a = vertices[i];
        const auto& b = vertices[i + 1];
        const auto& c = vertices[i + 2];
        Vec3 pa{a.x, a.y, a.z}, pb{b.x, b.y, b.z}, pc{c.x, c.y, c.z};
        assert(dot(normalized(cross(pb - pa, pc - pa)), {a.nx, a.ny, a.nz}) > 0.999f);
        assert(pa.x >= bounds.minimum.x && pa.x <= bounds.maximum.x);
        assert(pa.y >= bounds.minimum.y && pa.y <= bounds.maximum.y);
        assert(pa.z >= bounds.minimum.z && pa.z <= bounds.maximum.z);
    }
    assert(!sheep.damage(4) && sheep.damage(4));
    const auto stopped = sheep.body.feet;
    sheep.update(flat, 0.05f);
    assert(sheep.body.feet.x == stopped.x && sheep.body.feet.z == stopped.z);
    BlockStore blocks(1337);
    AnimalSystem system;
    system.spawn(blocks);
    assert(system.animals().size() == 3);
    const auto target = system.animals().front()->body.feet;
    Camera camera;
    camera.eye = target + Vec3{0, 0.7f, 2};
    Inventory inventory;
    const BlockPos wall{int(std::floor(target.x)), int(std::floor(target.y + 0.7f)),
                        int(std::floor(target.z + 1))};
    const auto previous = blocks.block(wall.x, wall.y, wall.z);
    blocks.set(wall, Block::Stone);
    assert(std::strcmp(system.attack(blocks, inventory, camera), "No animal in reach") == 0);
    blocks.set(wall, previous);
    system.attack(blocks, inventory, camera);
    system.attack(blocks, inventory, camera);
    assert(system.animals().size() == 2 && inventory.count(Item::Wool) == 2);
}
} // namespace
void testRecipes() {
    Inventory inventory;
    inventory.select(-1);
    assert(inventory.selected() == Item::Shield);
    inventory.select(1);
    assert(inventory.selected() == Item::Grass);
    assert(inventory.choose(Item::OakLog) && inventory.selectedBlock() == Block::OakLog);
    assert(Inventory::fromBlock(Block::BirchLeaves) == Item::BirchLeaves);
    RecipeBook book;
    CraftGrid grid;
    grid.fill(Item::Count);
    grid[3] = Item::OakLog;
    const auto* planks = book.match(grid, 2);
    assert(planks && planks->result == Item::Planks);
    assert(!book.craft(*planks, inventory));
    inventory.add(Item::OakLog, 1);
    assert(book.craft(*planks, inventory));
    assert(inventory.count(Item::OakLog) == 0 && inventory.count(Item::Planks) == 4);
    grid.fill(Item::Count);
    grid[4] = grid[7] = Item::Planks;
    const auto* sticks = book.match(grid, 3);
    assert(sticks && sticks->kind == RecipeKind::Shaped);
    assert(book.craft(*sticks, inventory) && inventory.count(Item::Stick) == 4 &&
           inventory.count(Item::Planks) == 2);
    inventory.add(Item::Stick, Inventory::StackLimit - 4);
    assert(!book.craft(*sticks, inventory) && inventory.count(Item::Planks) == 2);
    grid[0] = Item::Dirt;
    assert(!book.match(grid, 3));
    Recipe forged = *sticks;
    assert(!book.craft(forged, inventory));
    grid.fill(Item::Count);
    grid[0] = grid[1] = Item::Planks;
    assert(!book.match(grid, 2)); // Shape matters: sticks require a vertical pair.
    grid.fill(Item::Count);
    grid[0] = static_cast<Item>(255);
    assert(!book.match(grid, 2));
    grid[0] = Item::OakLog;
    assert(!book.match(grid, 2, static_cast<Release>(110)));
}
void testSurvival() {
    Survival state;
    Inventory inventory;
    inventory.add(Item::Sword);
    assert(state.attack(Item::Sword) == 6);
    assert(state.attack(Item::Sword) == 1);
    for (int n = 0; n < 7; ++n)
        state.tick(0.1f, GameMode::Survival, 0);
    assert(state.attack(Item::Sword) == 6);
    inventory.add(Item::Shield);
    assert(state.equipOffhand(inventory, Item::Shield));
    assert(inventory.count(Item::Shield) == 0);
    state.blocking = true;
    for (int n = 0; n < 3; ++n)
        state.tick(0.1f, GameMode::Survival, 0);
    assert(!state.damage(6, true, true, GameMode::Survival));
    assert(state.health == 20 && state.shieldDurability == 329);
    for (int n = 0; n < 4; ++n)
        state.tick(0.1f, GameMode::Survival, 0);
    assert(state.damage(6, true, false, GameMode::Survival) && state.health == 14);
    assert(!state.damage(6, false, false, GameMode::Creative));
    state.saturation = 0;
    state.exhaustion = 4;
    state.tick(0, GameMode::Survival, 0);
    assert(state.hunger == 19);
    inventory.add(Item::Pork);
    assert(state.eat(inventory, Item::Pork) && state.hunger == 20);
    assert(!state.eat(inventory, Item::Pork));
    state.hunger = 0;
    state.saturation = 0;
    const auto health = state.health;
    for (int n = 0; n < 42; ++n)
        state.tick(0.1f, GameMode::Survival, 0);
    assert(state.health < health);
    state.hurtTime = 0;
    assert(state.damage(100, false, false, GameMode::Survival) && state.dead());
    state.respawn();
    assert(state.health == 20 && state.hunger == 20 && state.offhand == Item::Count);
    state.health = 10;
    for (int n = 0; n < 42; ++n)
        state.tick(0.1f, GameMode::Survival, 0);
    assert(state.health == 11);
    RayHit hit{};
    hit.hit = true;
    hit.block = Block::Stone;
    hit.position = {0, 30, 0};
    assert(!state.mine(hit, Item::Count, 0.1f, GameMode::Survival));
    state.mine({}, Item::Count, 0.1f, GameMode::Survival);
    assert(state.miningTime == 0);
    bool mined = false;
    for (int n = 0; n < 6; ++n)
        mined |= state.mine(hit, Item::Pickaxe, 0.1f, GameMode::Survival);
    assert(mined);
    inventory.add(Item::Elytra);
    assert(state.equipElytra(inventory) && inventory.count(Item::Elytra) == 0);
    FlatWorld flat;
    Player player;
    player.feet = {0, 30, 0};
    state.gliding = true;
    state.glideVelocity = {0, -1, -9};
    for (int n = 0; n < 21; ++n)
        assert(state.glide(player, flat, 0.05f, GameMode::Survival));
    assert(player.feet.z < -5 && player.feet.y > 25 && state.elytraDurability == 431);
    state.elytraDurability = 1;
    assert(!state.glide(player, flat, 0.05f, GameMode::Survival));
    assert(state.equipElytra(inventory) && state.equipElytra(inventory));
    assert(state.elytraDurability == 1); // Re-equipping cannot repair worn wings.
    // A usable item cannot disappear into the voxel-placement path.
    BlockStore blocks(7);
    blocks.set({0, 60, 0}, Block::Stone);
    auto face = raycast(blocks, {0.5f, 60.5f, 2.5f}, {0, 0, -1});
    inventory.choose(Item::Sword);
    const auto edits = blocks.editCount();
    assert(std::string(placeBlock(blocks, inventory, face, nullptr, 0)) ==
           "Selected item cannot be placed");
    assert(inventory.count(Item::Sword) == 1 && blocks.editCount() == edits);
    RecipeBook book;
    for (const auto& recipe : book.recipes()) {
        Inventory bag;
        for (auto cost : recipe.ingredients)
            if (cost.item != Item::Count)
                bag.add(cost.item, cost.count);
        assert(book.craft(recipe, bag) && bag.count(recipe.result) == recipe.resultCount);
    }
    std::uint32_t random = 9;
    const auto first = randomWorldName(random), second = randomWorldName(random);
    assert(first != second && std::count(first.begin(), first.end(), '-') == 2);
    ZombieAnimal zombie({0, 0.02f, 0});
    zombie.target = {5, 0, 0};
    for (int n = 0; n < 60; ++n)
        zombie.update(flat, 1.0f / 60);
    assert(zombie.body.feet.x > 0.5f && zombie.drop().item == Item::IronIngot);
}
void testDimensions() {
    for (auto dim : {Dimension::Overworld, Dimension::Nether, Dimension::End}) {
        Terrain terrain(7, 3, dim);
        const auto arrival = safeArrival(terrain, dim, {});
        PhysicsBody player;
        player.feet = arrival;
        assert(!collides(terrain, player, arrival));
        assert(terrain.block(int(std::floor(arrival.x)), int(arrival.y) - 1,
                             int(std::floor(arrival.z))) != Block::Air);
        Column column;
        generateColumn(column, terrain, 0, 0);
        assert(!column.chunks.empty());
        for (const auto& chunk : column.chunks)
            assert(!chunk.vertices.empty());
    }
    Terrain flat(7, 3, Dimension::Overworld, WorldType::Superflat);
    assert(flat.block(500, 3, -500) == Block::Grass && flat.block(500, 4, -500) == Block::Air);
    Terrain end(7, 3, Dimension::End);
    assert(end.block(200, -1, 200) == Block::Air && end.block(200, 20, 200) == Block::Air);
    Player falling;
    falling.feet = {200, 0, 200};
    for (int n = 0; n < 100; ++n)
        falling.step(end, {}, 0.05f);
    assert(falling.feet.y < -16);
}
int main() {
    {
        Inventory inventory;
        std::array<unsigned, ItemCount> chest{};
        inventory.add(Item::Stone, 5);
        assert(transferChest(chest, inventory, Item::Stone, true, false));
        assert(chest[2] == 1 && inventory.count(Item::Stone) == 4);
        assert(transferChest(chest, inventory, Item::Stone, true, true));
        assert(chest[2] == 5 && inventory.count(Item::Stone) == 0);
        inventory.add(Item::Stone, Inventory::StackLimit - 2);
        assert(transferChest(chest, inventory, Item::Stone, false, true));
        assert(chest[2] == 3 && inventory.count(Item::Stone) == Inventory::StackLimit);
        assert(!transferChest(chest, inventory, Item::Stone, false, false));
        assert(chest[2] == 3);
    }
    testSurvival();
    testDimensions();
    {
        BlockStore world(42, 4);
        auto village = villageSite(world.terrain(), 0, 0);
        assert(village.valid);
        AnimalSystem animals;
        animals.updateVillages(world, {float(village.x), float(village.y), float(village.z)}, 1,
                               {.1f, .2f, .3f, .4f});
        assert(animals.animals().size() == 3);
        for (const auto& a : animals.animals()) {
            assert(a->species() == Species::Villager);
            std::vector<Vertex> vertices;
            a->appendModel(vertices);
            assert(vertices.size() == 6 * 36);
            for (auto v : vertices)
                assert(v.u >= .1f && v.u <= .3f && v.v >= .2f && v.v <= .4f);
        }
        animals.updateVillages(world, {800, 30, 800}, 1, {.1f, .2f, .3f, .4f});
        assert(animals.animals().empty());
    }
    {
        // Every item is reachable exactly once across native 27-cell pages.
        std::array<unsigned, ItemCount> seen{};
        for (unsigned page = 0; page < InventoryPages; ++page)
            for (unsigned cell = 0; cell < 27; ++cell) {
                auto item = inventoryPageItem(page, cell);
                if (item != Item::Count)
                    ++seen[unsigned(item)];
                assert(inventoryPageItem(page, cell + 27) == item);
            }
        for (auto n : seen)
            assert(n == 1);
        assert(inventoryPageItem(InventoryPages, 0) == Item::Count);
        assert(!canHarvest(Block::DiamondOre, Item::StonePickaxe));
        assert(canHarvest(Block::DiamondOre, Item::Pickaxe));
        assert(!canHarvest(Block::Obsidian, Item::Pickaxe));
        assert(canHarvest(Block::Obsidian, Item::DiamondPickaxe));
        assert(!canHarvest(Block::IronOre, Item::GoldPickaxe));
        Survival survival;
        Inventory bag;
        bag.add(Item::DiamondChestplate);
        assert(survival.equipArmor(bag, Item::DiamondChestplate));
        assert(survival.armorPoints() == 8 && !bag.count(Item::DiamondChestplate));
        assert(survival.damage(10, true, false, GameMode::Survival));
        assert(survival.health > 10 && survival.armorWear[1] == 2);
        assert(survival.equipArmor(bag, Item::DiamondChestplate));
        assert(survival.equipArmor(bag, Item::DiamondChestplate));
        assert(survival.armorWear[1] == 2); // Removing armor cannot repair it.
        bag.add(Item::Elytra);
        assert(!survival.equipElytra(bag));
        bag.add(Item::WoodPickaxe, 2);
        for (unsigned n = 0; n < 58; ++n)
            assert(!survival.wearTool(bag, Item::WoodPickaxe));
        assert(survival.wearTool(bag, Item::WoodPickaxe) && bag.count(Item::WoodPickaxe) == 1);
        assert(survival.toolWear[unsigned(Item::WoodPickaxe)] == 0);
        survival.attackAge = 1;
        auto normal = survival.attack(Item::Sword);
        survival.attackAge = 1;
        assert(survival.attack(Item::Sword, true) > normal);
        bag.add(Item::Steak);
        survival.hunger = 10;
        survival.saturation = 0;
        assert(survival.eat(bag, Item::Steak) && survival.hunger == 18 && survival.saturation > 12);
    }
    {
        Furnace f;
        Inventory bag;
        bag.add(Item::IronOre, 9);
        bag.add(Item::Coal);
        assert(!f.deposit(bag, Item::IronOre, true, true));
        assert(f.deposit(bag, Item::IronOre, false, true));
        assert(f.deposit(bag, Item::Coal, true, true));
        f.tick(9);
        assert(f.outputCount == 0 && f.progress == 9);
        f.tick(1);
        assert(f.output == Item::IronIngot && f.outputCount == 1 && f.inputCount == 8);
        f.tick(60);
        f.tick(10); // A coal cooks exactly eight items.
        assert(f.outputCount == 8 && f.inputCount == 1 && f.burn == 0);
        f.tick(20);
        assert(f.outputCount == 8 && f.progress == 0);
        bag.add(Item::IronIngot, Inventory::StackLimit);
        assert(!f.take(bag, 2) && f.outputCount == 8);
        bag.remove(Item::IronIngot, 4);
        assert(f.take(bag, 2) && f.outputCount == 4);
        assert(f.take(bag, 0) && f.input == Item::Count && f.inputCount == 0);
        assert(!f.take(bag, 3));
        Furnace obstructed;
        bag.add(Item::Coal);
        bag.add(Item::GoldOre);
        obstructed.deposit(bag, Item::GoldOre, false, true);
        obstructed.deposit(bag, Item::Coal, true, true);
        obstructed.output = Item::IronIngot;
        obstructed.outputCount = 1;
        obstructed.tick(10);
        assert(obstructed.fuelCount == 1 && obstructed.burn == 0);
    }
    {
        BlockStore world(42, 5, Dimension::Overworld, WorldType::Superflat);
        AnimalSystem animals;
        animals.spawn(world);
        animals.animals()[0]->body.feet = {0, 4.02f, -2};
        animals.animals()[1]->body.feet = {.8f, 4.02f, -2};
        animals.animals()[2]->body.feet = {5, 4.02f, -2};
        Camera camera;
        camera.eye = {0, 4.6f, 0};
        Inventory bag;
        auto hp = animals.animals()[1]->health.current;
        assert(std::string(animals.attack(world, bag, camera, 2, true)) == "Sweeping hit");
        assert(animals.animals()[0]->health.current == 6 &&
               animals.animals()[1]->health.current == hp - 1);
        assert(animals.animals()[2]->health.current == 12);
        // Full drops reject the complete sweep before any damage is applied.
        bag.add(Item::Wool, Inventory::StackLimit);
        auto primaryHp = animals.animals()[0]->health.current;
        hp = animals.animals()[1]->health.current;
        assert(std::string(animals.attack(world, bag, camera, 20, true)) == "Drop stack full");
        assert(animals.animals()[0]->health.current == primaryHp &&
               animals.animals()[1]->health.current == hp);
    }
    testRecipes();
    testEditsAndRays();
    testLimits();
    testCrouch();
    testTimeAndAnimals();
    std::cout << "Gameplay tests passed: edits/limits/halos, rays, inventory/placement/collision, "
                 "20-minute cycle, FPS, animal AI/models/health/drops\n";
}

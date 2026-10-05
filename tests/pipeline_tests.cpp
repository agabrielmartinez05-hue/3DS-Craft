#include "AssetCatalog.hpp"
#include "Controls.hpp"
#include "Error.hpp"
#include "Gui.hpp"
#include "Menu.hpp"
#include "WorldStore.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <png.h>

using namespace voxel;
namespace fs = std::filesystem;
namespace {
template <class F> void fails(F fn, const std::string& expected) {
    bool failed = false;
    try {
        fn();
    } catch (const std::exception& e) {
        failed = true;
        assert(std::string(e.what()).find(expected) != std::string::npos);
    }
    if (!failed)
        std::cerr << "Expected failure containing: " << expected << "\n";
    assert(failed);
}
void write(const fs::path& path, const std::string& text) {
    std::ofstream out(path);
    out << text;
}
void testPack(const fs::path& root) {
    AssetArchive cache;
    cache.preload("assets");
    assert(cache.fileCount() == 10968 && cache.bytes() == 5783889);
    assert(cache.contains("minecraft/models/block/grass_block.json"));
    assert(!cache.contains("minecraft/models/blocks/grass.json"));
    AssetCatalog catalog;
    Image atlas;
    unsigned uploads = 0;
    catalog.load(cache, "minecraft", {}, [&](unsigned id, const Image& pixels) {
        assert(id == 0);
        atlas = pixels;
        ++uploads;
    });
    assert(uploads == 1 && atlas.width == 512 && atlas.height == 512);
    for (unsigned a = 0; a < catalog.regions().size(); ++a) {
        const auto& r = catalog.regions()[a];
        assert(r.x >= 1 && r.y >= 1 && r.x + r.width < atlas.width &&
               r.y + r.height < atlas.height);
        for (unsigned b = a + 1; b < catalog.regions().size(); ++b) {
            const auto& q = catalog.regions()[b];
            assert(r.x + r.width + 1 <= q.x - 1 || q.x + q.width + 1 <= r.x - 1 ||
                   r.y + r.height + 1 <= q.y - 1 || q.y + q.height + 1 <= r.y - 1);
        }
    }
    assert(catalog.regions().size() <
           300); // Raw RAM cache is complete; GPU residency is demand-driven.
    for (const auto& tile : catalog.regions()) {
        const auto source = cache.png("minecraft/textures/" + tile.file);
        for (unsigned y = 0; y < tile.height; ++y)
            for (unsigned x = 0; x < tile.width; ++x)
                for (unsigned c = 0; c < 4; ++c)
                    assert(atlas.rgba[((tile.y + y) * atlas.width + tile.x + x) * 4 + c] ==
                           source.rgba[(y * source.width + x) * 4 + c]);
    }
    png_image output{};
    output.version = PNG_IMAGE_VERSION;
    output.width = atlas.width;
    output.height = atlas.height;
    output.format = PNG_FORMAT_RGBA;
    assert(png_image_write_to_file(&output, (root / "repository-atlas.png").c_str(), 0,
                                   atlas.rgba.data(), 0, nullptr));
    const auto& grass = catalog.models()[unsigned(Block::Grass)];
    assert(grass.occludes && grass.elements.size() == 2);
    assert(!grass.elements[0].faces[unsigned(Face::North)].cutout);
    assert(grass.elements[1].faces[unsigned(Face::North)].cutout);
    auto tint = grass.elements[0].faces[unsigned(Face::Up)].tint;
    assert(tint.y > tint.x && tint.y > tint.z);
    assert(catalog.models()[unsigned(Block::OakLeaves)].elements[0].faces[0].cutout);
    for (auto name : GuiSprites)
        assert(catalog.sprite(name) < catalog.regions().size());
    assert(catalog.regions()[catalog.sprite("hud/hotbar")].width == 182);
    assert(catalog.regions()[catalog.sprite("gui/container/inventory")].width == 176);
    for (unsigned i = 0; i < ItemCount; ++i)
        assert(catalog.item(static_cast<Item>(i)) < catalog.regions().size());
    assert(catalog.block(Block::Chest).opensInventory && !catalog.block(Block::Lava).breakable);
    const auto& lava = catalog.regions()[catalog.sprite("block/lava_still")];
    assert(lava.width == 16 && lava.height == 16);
    Terrain terrain(7, 4);
    Column column;
    generateColumn(column, terrain, 0, 0, {}, &catalog.models());
    unsigned vertices = 0;
    for (const auto& chunk : column.chunks) {
        unsigned end = 0;
        for (const auto& r : chunk.ranges) {
            assert(r.first == end && r.material <= 1);
            end += r.count;
        }
        assert(end == chunk.vertices.size());
        vertices += end;
    }
    assert(vertices > 0);
    // Real layered repository models must fit the live window around a village.
    std::size_t vboBytes = 0;
    Terrain village(42, 5);
    for (int z = 0; z < 5; ++z)
        for (int x = -1; x < 4; ++x) {
            Column col;
            generateColumn(col, village, x, z, {}, &catalog.models());
            for (const auto& chunk : col.chunks)
                vboBytes += chunk.vertices.size() * sizeof(Vertex);
        }
    assert(vboBytes < 12 * 1024 * 1024);
    std::cout << "Real village window VBO bytes: " << vboBytes << "\n";

    std::cout << "Actual repository: " << cache.fileCount() << " cached files, " << cache.bytes()
              << " raw bytes, " << catalog.regions().size() << " sprites, " << atlas.width
              << "px atlas\n";
    // Test strict path failure and cache-only decode on a copy of the real tree.
    const auto copy = root / "disk";
    fs::copy("assets", copy, fs::copy_options::recursive);
    fs::rename(copy / "minecraft/models/block", copy / "minecraft/models/blocks");
    AssetArchive broken;
    broken.preload(copy.string());
    fails([&] { catalog.load(broken, "minecraft", {}, {}); }, "block/");
    fs::rename(copy / "minecraft/models/blocks", copy / "minecraft/models/block");
    const auto hotbar = copy / "minecraft/textures/gui/sprites/hud/hotbar.png";
    fs::remove(hotbar);
    broken.preload(copy.string());
    fails([&] { catalog.load(broken, "minecraft", {}, {}); }, "hud/hotbar");
    fs::copy_file("assets/minecraft/textures/gui/sprites/hud/hotbar.png", hotbar);
    const auto stone = copy / "minecraft/models/block/stone.json";
    auto json = loadJson(stone.c_str());
    auto original = json;
    json["textures"]["all"] = "block/absent";
    write(stone, json.dump());
    broken.preload(copy.string());
    fails([&] { catalog.load(broken, "minecraft", {}, {}); }, "absent");
    write(stone, original.dump());
    broken.preload(copy.string());
    fs::remove_all(copy);
    catalog.load(broken, "minecraft", {},
                 {}); // Entire pack works after removing its source SD directory.
    assert(broken.fileCount() == 10968);
}
void testPixels() {
    Image source{16, 16, {}};
    source.rgba.resize(16 * 16 * 4);
    for (unsigned y = 0; y < 16; ++y)
        for (unsigned x = 0; x < 16; ++x) {
            const unsigned offset = (y * 16 + x) * 4;
            source.rgba[offset] = x;
            source.rgba[offset + 1] = y;
            source.rgba[offset + 2] = x + y;
            source.rgba[offset + 3] = 255;
        }
    const auto crop = extractRegion(source, {"synthetic", 8, 0, 8, 8});
    assert(crop.rgba[0] == 8 && crop.rgba[1] == 0 && crop.rgba[7 * 8 * 4 + 1] == 7);
    const auto tiled = tileRgba8(source);
    // Golden offsets for 8x8 Morton tiles + vertical flip + ABGR (not an encoder copy).
    assert(tiled[0] == 255 && tiled[1] == 15 && tiled[2] == 15 && tiled[3] == 0); // (0,15)
    assert(tiled[4 + 3] == 1 && tiled[4 + 2] == 15);                              // (1,15)
    assert(tiled[8 + 3] == 0 && tiled[8 + 2] == 14);                              // (0,14)
    assert(tiled[64 * 4 + 3] == 8 && tiled[64 * 4 + 2] == 15);                // Next tile in row.
    assert(tiled[(128 + 42) * 4 + 3] == 0 && tiled[(128 + 42) * 4 + 2] == 0); // Top-left original.
    fails([&] { extractRegion(source, {"bad", 9, 0, 8, 8}); }, "outside image");
    fails([&] { tileRgba8({7, 8, {}}); }, "Invalid image");
}
void testSaves(const fs::path& root) {
    WorldStore saves((root / "saves").string());
    assert(saves.list().empty());
    const auto id = saves.create("Test World", 4294967295u, 123);
    auto original = saves.load(id);
    assert(original.name == "Test World" && original.seed == UINT32_MAX &&
           original.generation == 1 && original.generator == 5);
    assert(saves.list().size() == 1 && saves.list()[0].valid);
    auto next = original;
    next.edits = {{{-17, 55, 16}, Block::Stone}, {{15, 15, 0}, Block::Air}};
    next.inventory[2] = 42;
    next.selected = 2;
    next.daySeconds = 900;
    next.crouched = true;
    saves.save(id, next);
    auto loaded = saves.load(id);
    assert(loaded.generation == 2 && loaded.edits.size() == 2 && loaded.inventory[2] == 42 &&
           loaded.daySeconds == 900 && loaded.crouched);
    // Simulate interrupted staging: .tmp is never treated as a committed generation.
    write(root / "saves" / id / "world.0.json.tmp", "{truncated");
    assert(saves.load(id).generation == 2);
    // Truncated newer generation recovers the last fully valid state.
    write(root / "saves" / id / "world.1.json", "{truncated");
    assert(saves.load(id).generation == 1 && saves.load(id).recovered);
    saves.save(id, next);
    assert(saves.load(id).generation == 2);
    auto invalid = next;
    invalid.inventory[0] = 1000;
    fails([&] { saves.save(id, invalid); }, "inventory");
    assert(saves.load(id).generation == 2);
    invalid = next;
    invalid.edits.push_back(next.edits[0]);
    fails([&] { saves.save(id, invalid); }, "Duplicate");
    fails([&] { saves.load("../outside"); }, "directory id");
    // Huge unsigned edit coordinates must not wrap to valid negative integers.
    auto json = loadJson((root / "saves" / id / "world.1.json").c_str());
    json["edits"][0][0] = UINT64_MAX;
    write(root / "saves" / id / "world.1.json", json.dump());
    assert(saves.load(id).generation == 1 && saves.load(id).recovered);
    write(root / "saves" / id / "world.0.json", "broken");
    assert(!saves.list()[0].valid);
    fails([&] { saves.load(id); }, "No valid save");
    fails([&] { saves.save(id, next); }, "No valid save");
    assert(parseSeed("0") == 0 && parseSeed("4294967295") == UINT32_MAX);
    fails([&] { parseSeed("4294967296"); }, "Seed");
    fails([&] { parseSeed("-1"); }, "Seed");
    fails([&] { saves.create("", 1, 124); }, "name");
    const auto oldId = saves.create("Legacy", 1337, 999);
    const auto oldPath = root / "saves" / oldId / "world.0.json";
    auto old = loadJson(oldPath.c_str());
    old["format"] = 1;
    old.erase("generator");
    old["inventory"] = {16u, 16u, 16u, 0u, 0u, 0u};
    write(oldPath, old.dump());
    auto legacy = saves.load(oldId);
    assert(legacy.generator == 1 && legacy.inventory[unsigned(Item::OakLog)] == 0);
    saves.save(oldId, legacy);
    assert(saves.load(oldId).generator == 1);
    assert(saves.create("Test World", 1, 123) != id); // Same nonce cannot overwrite a world.
}
void testSurvivalSaves(const fs::path& root) {
    WorldStore saves((root / "survival-saves").string());
    fails([&] { saves.create("Invalid", 7, 1, GameMode::Survival, WorldType::Superflat); },
          "Superflat");
    auto id = saves.create("Brave-Oak-Valley", 7, 1, GameMode::Creative, WorldType::Superflat);
    auto save = saves.load(id);
    assert(save.type == WorldType::Superflat && save.feet.y < 5 && save.mode == GameMode::Creative);
    id = saves.create("World 🌍", 7, 2);
    save = saves.load(id);
    assert(save.mode == GameMode::Survival && save.inventory[0] == 0);
    save.dimension = Dimension::Nether;
    save.feet = safeArrival(Terrain(7, 3, Dimension::Nether), Dimension::Nether, {});
    save.edits = {{{2, 30, 2}, Block::Obsidian}};
    save.journals[0] = {{{3, 50, 3}, Block::Stone}};
    save.journals[2] = {{{4, 40, 4}, Block::EndStone}};
    save.visited[1] = true;
    save.survival.health = 7;
    save.survival.hunger = 3;
    save.survival.offhand = Item::Shield;
    save.survival.shieldDurability = 12;
    save.survival.elytra = true;
    save.survival.elytraDurability = 29;
    save.dropActive = true;
    save.dropDimension = Dimension::End;
    save.dropPosition = {1, 30, 1};
    save.dropped[0] = 5;
    saves.save(id, save);
    auto loaded = saves.load(id);
    assert(loaded.dimension == Dimension::Nether && loaded.journals[0].size() == 1 &&
           loaded.journals[2].size() == 1);
    assert(loaded.edits[0].value == Block::Obsidian && loaded.survival.health == 7 &&
           loaded.survival.hunger == 3);
    assert(loaded.survival.offhand == Item::Shield && loaded.survival.shieldDurability == 12 &&
           loaded.survival.elytraDurability == 29);
    assert(loaded.dropActive && loaded.dropped[0] == 5 && loaded.dropDimension == Dimension::End);
    auto invalid = save;
    invalid.survival.health = 21;
    fails([&] { saves.save(id, invalid); }, "health");
    assert(saves.load(id).generation == 2);
    // Three full bounded journals still fit the loader's 1 MiB save limit.
    save.edits.clear();
    for (unsigned d = 0; d < 3; ++d) {
        save.journals[d].clear();
        for (unsigned n = 0; n < BlockStore::MaxEdits; ++n)
            save.journals[d].push_back({{int(n % 128), 60, int(n / 128)}, Block::Stone});
    }
    save.edits = save.journals[1];
    saves.save(id, save);
    assert(saves.load(id).journals[2].size() == BlockStore::MaxEdits);
    save.chests[0][{4, 30, 4}][unsigned(Item::OakLog)] = 12;
    saves.save(id, save);
    assert(saves.load(id).chests[0].at({4, 30, 4})[unsigned(Item::OakLog)] == 12);
    invalid = save;
    invalid.chests[0][{4, 30, 4}][0] = 1000;
    fails([&] { saves.save(id, invalid); }, "chest count");
    save.survival.armor[0] = Item::DiamondHelmet;
    save.survival.armorWear[0] = 20;
    save.survival.toolWear[unsigned(Item::DiamondPickaxe)] = 100;
    auto& furnace = save.furnaces[2][{5, 31, 5}];
    furnace.input = Item::IronOre;
    furnace.inputCount = 3;
    furnace.output = Item::IronIngot;
    furnace.outputCount = 2;
    furnace.burn = 20;
    furnace.progress = 4;
    saves.save(id, save);
    const auto equipment = saves.load(id);
    assert(equipment.survival.armorWear[0] == 20 &&
           equipment.survival.armor[0] == Item::DiamondHelmet);
    assert(equipment.survival.toolWear[unsigned(Item::DiamondPickaxe)] == 100);
    assert(equipment.furnaces[2].at({5, 31, 5}).progress == 4);
    invalid = save;
    invalid.survival.armor[0] = Item::DiamondBoots;
    fails([&] { saves.save(id, invalid); }, "Armor in invalid");
    invalid = save;
    invalid.furnaces[2].at({5, 31, 5}).inputCount = 0;
    fails([&] { saves.save(id, invalid); }, "Furnace slot mismatch");
    auto version4 = saves.create("Previous native", 42, 998);
    auto version4file = root / "survival-saves" / version4 / "world.0.json";
    auto v4 = loadJson(version4file.c_str());
    v4["format"] = 4;
    v4["generator"] = 4;
    v4["survival"]["offhand"] = 24u;
    v4["inventory"].erase(v4["inventory"].begin() + 24, v4["inventory"].end());
    v4["drop"]["items"].erase(v4["drop"]["items"].begin() + 24, v4["drop"]["items"].end());
    v4["chests"][0].push_back({{"position", {4, 30, 4}}, {"items", std::array<unsigned, 24>{}}});
    v4["chests"][0][0]["items"][23] = 7u;
    write(version4file, v4.dump());
    auto oldNative = saves.load(version4);
    assert(oldNative.generator == 4 && oldNative.survival.offhand == Item::Count);
    assert(oldNative.chests[0].at({4, 30, 4})[23] == 7 && oldNative.inventory[24] == 0);
    saves.save(version4, oldNative);
    assert(saves.load(version4).generator == 4);
    auto oldId = saves.create("Previous survival", 7, 999);
    auto file = root / "survival-saves" / oldId / "world.0.json";
    auto legacy = loadJson(file.c_str());
    legacy["format"] = 3;
    legacy["generator"] = 3;
    legacy["survival"]["offhand"] = 21u;
    legacy["inventory"].erase(legacy["inventory"].begin() + 21, legacy["inventory"].end());
    legacy["drop"]["items"].erase(legacy["drop"]["items"].begin() + 21,
                                  legacy["drop"]["items"].end());
    legacy.erase("chests");
    write(file, legacy.dump());
    auto migrated = saves.load(oldId);
    assert(migrated.generator == 3 && migrated.chests[0].empty() &&
           migrated.survival.offhand == Item::Count);
    saves.save(oldId, migrated);
    assert(saves.load(oldId).generator == 3);
}
void testEditorAndCalibration() {
    TextEditor editor;
    editor.open("", true);
    assert(editor.keys().size() == 13);
    editor.press(0);
    editor.press(9);
    assert(editor.text() == "10");
    editor.press(10);
    assert(editor.text() == "1");
    assert(editor.press(12));
    editor.press(11);
    assert(editor.text().empty());
    editor.open(std::string(32, 'a'), false);
    editor.press(0);
    assert(editor.text().size() == 32);
    editor.open("World 🌍", false);
    editor.press(unsigned(editor.keys().size() - 3));
    assert(editor.text() == "World ");
    ControlSettings settings;
    auto move = mapAnalog({156, 156, 0, 0}, settings);
    assert(move.forward == 1 && move.strafe == 1 && move.lookX == 0 && move.lookY == 0);
    auto look = mapAnalog({0, 0, 156, 156}, settings);
    assert(look.forward == 0 && look.strafe == 0 && look.lookX == 1 && look.lookY == 1);
    auto negative = mapAnalog({-156, -156, -156, -156}, settings);
    assert(negative.forward == -1 && negative.strafe == -1 && negative.lookX == -1 &&
           negative.lookY == -1);
    assert(calibratedAxis(15, 15) == 0 && calibratedAxis(-15, 15) == 0);
    assert(calibratedAxis(156, 15) == 1 && calibratedAxis(-156, 15) == -1);
    assert(calibratedAxis(60, 15) > 0 && calibratedAxis(60, 15) < 1);
}
} // namespace
int main(int argc, char** argv) {
    assert(argc == 2);
    const fs::path root = argv[1];
    fs::remove_all(root);
    fs::create_directories(root);
    testPack(root);
    testPixels();
    testSaves(root);

    testSurvivalSaves(root);
    testEditorAndCalibration();
    std::cout << "Pipeline tests passed: scan/schema/crop/swizzle, durable save "
                 "generations/recovery, invalid-data rejection, editor/calibration\n";
}

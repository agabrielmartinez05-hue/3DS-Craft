#include "WorldStore.hpp"
#include "Assets.hpp"
#include "Error.hpp"
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <limits>
#include <set>
#include <sys/stat.h>

namespace voxel {
namespace {
using Json = nlohmann::json;
Json encodeEdits(const std::vector<BlockEdit>& edits) {
    Json out = Json::array();
    for (const auto& e : edits)
        out.push_back({e.position.x, e.position.y, e.position.z, unsigned(e.value)});
    return out;
}
std::vector<BlockEdit> decodeEdits(const Json& edits, unsigned maxBlock) {
    std::vector<BlockEdit> result;
    if (!edits.is_array() || edits.size() > BlockStore::MaxEdits)
        throw Error("Invalid or oversized edit journal");
    std::set<BlockPos> positions;
    for (const auto& e : edits) {
        if (!e.is_array() || e.size() != 4)
            throw Error("Each edit requires [x,y,z,block]");
        for (const auto& v : e)
            if (!v.is_number_integer() ||
                (v.is_number_unsigned() && v.get<std::uint64_t>() > INT64_MAX))
                throw Error("Invalid edit integer");
        const auto x = e[0].get<std::int64_t>(), y = e[1].get<std::int64_t>(),
                   z = e[2].get<std::int64_t>(), b = e[3].get<std::int64_t>();
        if (x < -8192 || x >= 8192 || z < -8192 || z >= 8192 || y < 0 || y >= WorldHeight ||
            b < 0 || b >= maxBlock)
            throw Error("Edit out of bounds");
        BlockPos p{int(x), int(y), int(z)};
        if (!positions.insert(p).second)
            throw Error("Duplicate block edit coordinate");
        result.push_back({p, static_cast<Block>(b)});
    }
    return result;
}
unsigned bounded(const Json& j, unsigned maximum, const char* field) {
    if (!j.is_number_unsigned() || j.get<std::uint64_t>() > maximum)
        throw Error("Invalid %s", field);
    return j.get<unsigned>();
}
float real(const Json& j, float low, float high, const char* field) {
    if (!j.is_number())
        throw Error("Invalid %s", field);
    const float f = j.get<float>();
    if (!std::isfinite(f) || f < low || f > high)
        throw Error("Invalid %s", field);
    return f;
}
Vec3 position(const Json& j) {
    if (!j.is_array() || j.size() != 3)
        throw Error("Invalid saved position");
    return {real(j[0], -8191.7f, 8191.7f, "position x"), real(j[1], -128, 80, "position y"),
            real(j[2], -8191.7f, 8191.7f, "position z")};
}
Json pos(Vec3 p) {
    return {p.x, p.y, p.z};
}
WorldSave decode(const Json& json) {
    const auto format = json.at("format");
    if (format != 1 && format != 2 && format != 3 && format != 4 && format != 5)
        throw Error("Unsupported world save format (expected 1 through 5)");
    const unsigned savedItems = format == 1   ? 6u
                                : format == 2 ? 12u
                                : format == 3 ? 21u
                                : format == 4 ? 24u
                                              : ItemCount;
    const unsigned savedBlocks = format == 1   ? 4u
                                 : format == 2 ? 9u
                                 : format == 3 ? 14u
                                 : format == 4 ? 17u
                                               : BlockCount;
    WorldSave world;
    if (format != 1 && (!json.at("generator").is_number_unsigned() ||
                        json.at("generator").get<std::uint64_t>() > 5))
        throw Error("Unsupported terrain generator");
    world.generator = format == 1 ? 1 : json.at("generator").get<unsigned>();
    if (world.generator != 1 && world.generator != 2 && world.generator != 3 &&
        world.generator != 4 && world.generator != 5)
        throw Error("Unsupported terrain generator");
    world.inventory.fill(0);
    world.name = json.at("name").get<std::string>();
    if (world.name.empty() || world.name.size() > 128)
        throw Error("World name must have 1..128 UTF-8 bytes");
    for (unsigned char c : world.name)
        if (c < 32 || c == 127)
            throw Error("World name must not contain control characters");
    if (!json.at("seed").is_number_unsigned() || json.at("seed").get<std::uint64_t>() > UINT32_MAX)
        throw Error("World seed must be uint32");
    world.seed = json.at("seed").get<std::uint32_t>();
    if (!json.at("generation").is_number_unsigned())
        throw Error("Invalid save generation");
    world.generation = json.at("generation").get<std::uint64_t>();
    const auto& player = json.at("player");
    const auto& p = player.at("feet");
    if (!p.is_array() || p.size() != 3)
        throw Error("player.feet must have three coordinates");
    world.feet = {p[0].get<float>(), p[1].get<float>(), p[2].get<float>()};
    world.yaw = player.at("yaw").get<float>();
    world.pitch = player.at("pitch").get<float>();
    world.crouched = player.at("crouched").get<bool>();
    world.daySeconds = json.at("day_seconds").get<double>();
    if (!std::isfinite(world.feet.x) || !std::isfinite(world.feet.y) ||
        !std::isfinite(world.feet.z) || std::abs(world.feet.x) > WorldLimit - 0.3f ||
        std::abs(world.feet.z) > WorldLimit - 0.3f || world.feet.y < (format >= 3 ? -128 : 0) ||
        world.feet.y > WorldHeight + 16 || !std::isfinite(world.yaw) ||
        std::abs(world.yaw) > Pi + 0.01f || !std::isfinite(world.pitch) ||
        std::abs(world.pitch) > 1.5f || !std::isfinite(world.daySeconds) || world.daySeconds < 0 ||
        world.daySeconds >= DayNight::DaySeconds)
        throw Error("Invalid player coordinates, orientation, or day time");
    const auto& inventory = json.at("inventory");
    if (!inventory.is_array() || inventory.size() != savedItems)
        throw Error("inventory count array does not match save format");
    for (unsigned i = 0; i < inventory.size(); ++i) {
        if (!inventory[i].is_number_unsigned() ||
            inventory[i].get<std::uint64_t>() > Inventory::StackLimit)
            throw Error("Invalid inventory count in slot %u", i);
        world.inventory[i] = inventory[i].get<unsigned>();
    }
    if (!json.at("selected").is_number_unsigned() ||
        json.at("selected").get<std::uint64_t>() >= savedItems)
        throw Error("Invalid selected item");
    world.selected = json.at("selected").get<unsigned>();

    world.edits = decodeEdits(json.at("edits"), savedBlocks);
    if (format >= 3) {
        world.mode = static_cast<GameMode>(bounded(json.at("mode"), 1, "mode"));
        world.type = static_cast<WorldType>(bounded(json.at("world_type"), 1, "world type"));
        if (world.mode == GameMode::Survival && world.type == WorldType::Superflat)
            throw Error("Superflat requires Creative mode");
        world.dimension = static_cast<Dimension>(bounded(json.at("dimension"), 2, "dimension"));
        const auto& dims = json.at("dimensions");
        if (!dims.is_array() || dims.size() != 3)
            throw Error("Three dimension journals required");
        for (unsigned i = 0; i < 3; ++i) {
            world.journals[i] = decodeEdits(dims[i].at("edits"), savedBlocks);
            world.arrivals[i] = position(dims[i].at("arrival"));
            world.visited[i] = dims[i].at("visited").get<bool>();
        }
        const auto& v = json.at("survival");
        auto& state = world.survival;
        state.health = real(v.at("health"), 0, 20, "health");
        state.hunger = real(v.at("hunger"), 0, 20, "hunger");
        state.saturation = real(v.at("saturation"), 0, 20, "saturation");
        state.exhaustion = real(v.at("exhaustion"), 0, 64, "exhaustion");
        const auto offhand = bounded(v.at("offhand"), savedItems, "offhand");
        state.offhand = offhand == (savedItems) ? Item::Count : static_cast<Item>(offhand);
        state.elytra = v.at("elytra").get<bool>();
        state.shieldDurability = bounded(v.at("shield_durability"), 336, "shield durability");
        state.elytraDurability = bounded(v.at("elytra_durability"), 432, "elytra durability");
        if (format >= 5) {
            const auto& armor = v.at("armor");
            const auto& wear = v.at("armor_wear");
            const auto& tools = v.at("tool_wear");
            if (!armor.is_array() || armor.size() != 4 || !wear.is_array() || wear.size() != 4 ||
                !tools.is_array() || tools.size() != ItemCount)
                throw Error("Invalid equipment arrays");
            for (unsigned i = 0; i < 4; ++i) {
                state.armor[i] = static_cast<Item>(bounded(armor[i], ItemCount, "armor item"));
                const auto stats = armorStats(state.armor[i]);
                if (state.armor[i] != Item::Count &&
                    (stats.slot != int(i) || (i == 1 && state.elytra)))
                    throw Error("Armor in invalid equipment slot");
                state.armorWear[i] =
                    bounded(wear[i], stats.durability ? stats.durability - 1 : 0, "armor wear");
            }
            for (unsigned i = 0; i < ItemCount; ++i) {
                const auto durability = itemDurability(static_cast<Item>(i));
                state.toolWear[i] = bounded(tools[i], durability ? durability - 1 : 0, "tool wear");
            }
        }
        const auto& drop = json.at("drop");
        world.dropActive = drop.at("active").get<bool>();
        world.dropPosition = position(drop.at("position"));
        world.dropDimension =
            static_cast<Dimension>(bounded(drop.at("dimension"), 2, "drop dimension"));
        const auto& items = drop.at("items");
        if (!items.is_array() || items.size() != (savedItems))
            throw Error("Invalid dropped inventory");
        for (unsigned i = 0; i < items.size(); ++i)
            world.dropped[i] = bounded(items[i], Inventory::StackLimit, "dropped count");
    } else {
        world.mode = GameMode::Creative;
        world.arrivals[0] = world.feet;
    }
    if (format >= 4) {
        const auto& containers = json.at("chests");
        if (!containers.is_array() || containers.size() != 3)
            throw Error("Three chest dimensions required");
        for (unsigned d = 0; d < 3; ++d) {
            if (!containers[d].is_array() || containers[d].size() > 64)
                throw Error("Chest save limit is 64 per dimension");
            for (const auto& chest : containers[d]) {
                auto pos = position(chest.at("position"));
                BlockPos key{int(pos.x), int(pos.y), int(pos.z)};
                if (pos.x != key.x || pos.y != key.y || pos.z != key.z ||
                    !BlockStore::editable(key))
                    throw Error("Invalid chest position");
                std::array<unsigned, ItemCount> counts{};
                const auto& items = chest.at("items");
                if (!items.is_array() || items.size() != savedItems)
                    throw Error("Invalid chest item array");
                for (unsigned n = 0; n < savedItems; ++n)
                    counts[n] = bounded(items[n], Inventory::StackLimit, "chest count");
                if (!world.chests[d].emplace(key, counts).second)
                    throw Error("Duplicate chest position");
            }
        }
    }
    if (format >= 5) {
        const auto& dims = json.at("furnaces");
        if (!dims.is_array() || dims.size() != 3)
            throw Error("Three furnace dimensions required");
        for (unsigned d = 0; d < 3; ++d) {
            if (!dims[d].is_array() || dims[d].size() > 64)
                throw Error("Furnace limit is 64 per dimension");
            for (const auto& row : dims[d]) {
                auto p = position(row.at("position"));
                BlockPos key{int(p.x), int(p.y), int(p.z)};
                if (p.x != key.x || p.y != key.y || p.z != key.z || !BlockStore::editable(key))
                    throw Error("Invalid furnace position");
                Furnace f;
                const auto& items = row.at("items");
                const auto& counts = row.at("counts");
                if (!items.is_array() || items.size() != 3 || !counts.is_array() ||
                    counts.size() != 3)
                    throw Error("Invalid furnace slots");
                Item* types[] = {&f.input, &f.fuel, &f.output};
                unsigned* amounts[] = {&f.inputCount, &f.fuelCount, &f.outputCount};
                for (unsigned i = 0; i < 3; ++i) {
                    *types[i] = static_cast<Item>(bounded(items[i], ItemCount, "furnace item"));
                    *amounts[i] = bounded(counts[i], Inventory::StackLimit, "furnace count");
                    if ((*types[i] == Item::Count) != (*amounts[i] == 0))
                        throw Error("Furnace slot mismatch");
                }
                if ((f.inputCount && smeltingResult(f.input) == Item::Count) ||
                    (f.fuelCount && !fuelSeconds(f.fuel)))
                    throw Error("Invalid furnace ingredient/fuel");
                f.burn = real(row.at("burn"), 0, 800, "furnace burn");
                f.progress = real(row.at("progress"), 0, 9.99999f, "furnace progress");
                if (!f.inputCount && f.progress > 0)
                    throw Error("Furnace progress without input");
                if (!world.furnaces[d].emplace(key, f).second)
                    throw Error("Duplicate furnace position");
            }
        }
    }
    world.journals[unsigned(world.dimension)] = world.edits;
    return world;
}
Json encode(const WorldSave& world, std::uint64_t generation) {
    Json edits = Json::array();
    for (const auto& edit : world.edits)
        edits.push_back({edit.position.x, edit.position.y, edit.position.z, unsigned(edit.value)});
    Json dimensions = Json::array();
    for (unsigned i = 0; i < 3; ++i)
        dimensions.push_back(
            {{"edits",
              encodeEdits(i == unsigned(world.dimension) ? world.edits : world.journals[i])},
             {"arrival", pos(world.arrivals[i])},
             {"visited", world.visited[i]}});
    Json chests = Json::array();
    for (const auto& dimension : world.chests) {
        Json entries = Json::array();
        for (const auto& chest : dimension)
            entries.push_back({{"position", {chest.first.x, chest.first.y, chest.first.z}},
                               {"items", chest.second}});
        chests.push_back(std::move(entries));
    }
    Json furnaces = Json::array();
    for (const auto& dimension : world.furnaces) {
        Json rows = Json::array();
        for (const auto& entry : dimension) {
            const auto& p = entry.first;
            const auto& f = entry.second;
            rows.push_back({{"position", {p.x, p.y, p.z}},
                            {"items", {unsigned(f.input), unsigned(f.fuel), unsigned(f.output)}},
                            {"counts", {f.inputCount, f.fuelCount, f.outputCount}},
                            {"burn", f.burn},
                            {"progress", f.progress}});
        }
        furnaces.push_back(std::move(rows));
    }
    return {{"format", 5},
            {"furnaces", furnaces},
            {"chests", chests},
            {"generator", world.generator},
            {"mode", unsigned(world.mode)},
            {"world_type", unsigned(world.type)},
            {"dimension", unsigned(world.dimension)},
            {"dimensions", dimensions},
            {"survival",
             {{"health", world.survival.health},
              {"hunger", world.survival.hunger},
              {"saturation", world.survival.saturation},
              {"exhaustion", world.survival.exhaustion},
              {"offhand", unsigned(world.survival.offhand)},
              {"elytra", world.survival.elytra},
              {"shield_durability", world.survival.shieldDurability},
              {"elytra_durability", world.survival.elytraDurability},
              {"armor",
               {unsigned(world.survival.armor[0]), unsigned(world.survival.armor[1]),
                unsigned(world.survival.armor[2]), unsigned(world.survival.armor[3])}},
              {"armor_wear", world.survival.armorWear},
              {"tool_wear", world.survival.toolWear}}},
            {"drop",
             {{"active", world.dropActive},
              {"position", pos(world.dropPosition)},
              {"dimension", unsigned(world.dropDimension)},
              {"items", world.dropped}}},
            {"name", world.name},
            {"seed", world.seed},
            {"generation", generation},
            {"player",
             {{"feet", {world.feet.x, world.feet.y, world.feet.z}},
              {"yaw", world.yaw},
              {"pitch", world.pitch},
              {"crouched", world.crouched}}},
            {"day_seconds", world.daySeconds},
            {"inventory", world.inventory},
            {"selected", world.selected},
            {"edits", std::move(edits)}};
}
} // namespace
std::uint32_t parseSeed(const std::string& text) {
    if (text.empty() || text.size() > 10)
        throw Error("Seed must be 0..4294967295");
    std::uint64_t value = 0;
    for (char c : text) {
        if (c < '0' || c > '9')
            throw Error("Seed must contain decimal digits only");
        value = value * 10 + unsigned(c - '0');
        if (value > UINT32_MAX)
            throw Error("Seed must be 0..4294967295");
    }
    return static_cast<std::uint32_t>(value);
}
std::string WorldStore::directory(const std::string& id) const {
    if (!safeRelativePath(id) || id.find('/') != std::string::npos || id.size() > 80)
        throw Error("Invalid world directory id");
    return root_ + "/" + id;
}
WorldStore::Latest WorldStore::latest(const std::string& id) const {
    Latest result;
    std::string errors;
    for (int slot = 0; slot < 2; ++slot) {
        const auto file = directory(id) + "/world." + std::to_string(slot) + ".json";
        if (!fileExists(file))
            continue;
        result.anyFile = true;
        try {
            auto candidate = decode(loadJson(file.c_str()));
            if (result.slot < 0 || candidate.generation > result.save.generation) {
                result.save = std::move(candidate);
                result.slot = slot;
            }
        } catch (const std::bad_alloc&) {
            throw;
        } catch (const std::exception& e) {
            errors += file + ": " + e.what() + "\n";
        }
    }
    if (result.anyFile && result.slot < 0)
        throw Error("No valid save generation:\n%s", errors.c_str());
    result.save.recovered = !errors.empty();
    return result;
}
WorldSave WorldStore::load(const std::string& id) const {
    auto result = latest(id);
    if (result.slot < 0)
        throw Error("World '%s' has no committed save", id.c_str());
    return std::move(result.save);
}
void WorldStore::save(const std::string& id, const WorldSave& world) const {
    const auto previous = latest(id);
    if (previous.slot >= 0 && previous.save.generation == UINT64_MAX)
        throw Error("Save generation counter exhausted");
    const auto json = encode(world, previous.slot < 0 ? 1 : previous.save.generation + 1);
    // Validate the serialized representation before touching either committed slot.
    decode(Json::parse(json.dump()));
    const std::string data = json.dump() + "\n";
    if (data.size() > 1024 * 1024)
        throw Error("World save exceeds 1 MiB");
    const int target = previous.slot == 0 ? 1 : 0;
    writeFileDurably(directory(id) + "/world." + std::to_string(target) + ".json", data);
}
std::string WorldStore::create(const std::string& name, std::uint32_t seed, std::uint64_t nonce,
                               GameMode mode, WorldType type) {
    ensureDirectory(root_);
    unsigned directories = 0;
    for (const auto& entry : listDirectory(root_))
        if (entry.directory)
            ++directories;
    if (directories >= 64)
        throw Error("World limit reached (64 directories)");
    WorldSave world;
    world.name = name;
    world.seed = seed;
    world.mode = mode;
    world.type = type;
    if (mode == GameMode::Survival && type == WorldType::Superflat)
        throw Error("Superflat requires Creative mode");
    if (mode == GameMode::Survival)
        world.inventory.fill(0);
    else
        world.inventory.fill(64);
    Terrain terrain(seed, world.generator, Dimension::Overworld, type);
    Player player;
    player.spawn(terrain);
    world.feet = safeArrival(terrain, Dimension::Overworld, player.feet);
    world.arrivals[0] = world.feet;
    decode(Json::parse(encode(world, 1).dump()));
    for (unsigned attempt = 0; attempt < 64; ++attempt) {
        char id[40];
        std::snprintf(id, sizeof(id), "world-%016llx",
                      static_cast<unsigned long long>(nonce + attempt));
        const auto path = directory(id);
        if (mkdir(path.c_str(), 0777) == 0) {
            save(id, world);
            return id;
        }
        if (errno != EEXIST)
            throw Error("Cannot create world directory '%s'", path.c_str());
    }
    throw Error("Unable to allocate a unique world directory");
}
std::vector<WorldSummary> WorldStore::list(const Progress& progress) const {
    ensureDirectory(root_);
    std::vector<WorldSummary> worlds;
    for (const auto& entry : listDirectory(root_)) {
        if (!entry.directory)
            continue;
        if (worlds.size() >= 64)
            throw Error("World selection supports at most 64 save directories");
        if (progress)
            progress("Reading world " + entry.name, unsigned(worlds.size()), 0);
        WorldSummary summary{entry.name, entry.name, "", 0, false};
        try {
            auto world = load(entry.name);
            summary.name = world.name + (world.recovered ? " [recovered]" : "");
            summary.seed = world.seed;
            summary.valid = true;
        } catch (const std::bad_alloc&) {
            throw;
        } catch (const std::exception& e) {
            summary.error = e.what();
        }
        worlds.push_back(std::move(summary));
    }
    return worlds;
}
} // namespace voxel

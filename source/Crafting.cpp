#include "Crafting.hpp"
#include <algorithm>
#include <cmath>
namespace voxel {
RecipeBook::RecipeBook() {
    for (auto log : {Item::OakLog, Item::BirchLog}) {
        Recipe r;
        r.id = std::string("minecraft:planks_from_") + ItemTypes[unsigned(log)].id;
        r.ingredients[0] = {log, 1};
        r.result = log == Item::BirchLog ? Item::BirchPlanks : Item::Planks;
        r.resultCount = 4;
        recipes_.push_back(r);
    }
    Recipe sticks;
    sticks.id = "minecraft:sticks";
    sticks.kind = RecipeKind::Shaped;
    sticks.width = 1;
    sticks.height = 2;
    sticks.ingredients[0] = {Item::Planks, 1};
    sticks.ingredients[1] = {Item::Planks, 1};
    sticks.result = Item::Stick;
    sticks.resultCount = 4;
    recipes_.push_back(sticks);
    const auto add = [&](const char* id, Item result, unsigned amount,
                         std::initializer_list<Ingredient> cost, Release since = Release::V1_0) {
        Recipe r;
        r.id = id;
        r.result = result;
        r.resultCount = amount;
        r.since = static_cast<Release>(
            std::max(unsigned(since), unsigned(ItemTypes[unsigned(result)].since)));
        std::copy(cost.begin(), cost.end(), r.ingredients.begin());
        recipes_.push_back(r);
    };
    add("minecraft:shield", Item::Shield, 1, {{Item::IronIngot, 1}, {Item::Planks, 6}},
        Release::V1_9);
    const auto shape = [&](const char* id, Item result, unsigned count, unsigned width,
                           unsigned height, const char* cells, Item material) {
        Recipe r;
        r.id = id;
        r.result = result;
        r.resultCount = count;
        r.since = ItemTypes[unsigned(result)].since;
        r.kind = RecipeKind::Shaped;
        r.width = width;
        r.height = height;
        for (unsigned i = 0; i < width * height; ++i)
            r.ingredients[i] = {cells[i] == 'm'   ? material
                                : cells[i] == 's' ? Item::Stick
                                                  : Item::Count,
                                1};
        recipes_.push_back(r);
    };
    shape("minecraft:wood_sword", Item::WoodSword, 1, 1, 3, "mms", Item::Planks);
    shape("minecraft:wood_pickaxe", Item::WoodPickaxe, 1, 3, 3, "mmm s  s ", Item::Planks);
    shape("minecraft:wood_axe", Item::WoodAxe, 1, 2, 3, "mmms s", Item::Planks);
    shape("minecraft:wood_shovel", Item::WoodShovel, 1, 1, 3, "mss", Item::Planks);
    shape("minecraft:wood_hoe", Item::WoodHoe, 1, 2, 3, "mm s s", Item::Planks);
    shape("minecraft:stone_sword", Item::StoneSword, 1, 1, 3, "mms", Item::Cobblestone);
    shape("minecraft:stone_pickaxe", Item::StonePickaxe, 1, 3, 3, "mmm s  s ", Item::Cobblestone);
    shape("minecraft:stone_axe", Item::StoneAxe, 1, 2, 3, "mmms s", Item::Cobblestone);
    shape("minecraft:stone_shovel", Item::StoneShovel, 1, 1, 3, "mss", Item::Cobblestone);
    shape("minecraft:stone_hoe", Item::StoneHoe, 1, 2, 3, "mm s s", Item::Cobblestone);
    shape("minecraft:iron_sword", Item::Sword, 1, 1, 3, "mms", Item::IronIngot);
    shape("minecraft:iron_pickaxe", Item::Pickaxe, 1, 3, 3, "mmm s  s ", Item::IronIngot);
    shape("minecraft:iron_axe", Item::IronAxe, 1, 2, 3, "mmms s", Item::IronIngot);
    shape("minecraft:iron_shovel", Item::IronShovel, 1, 1, 3, "mss", Item::IronIngot);
    shape("minecraft:iron_hoe", Item::IronHoe, 1, 2, 3, "mm s s", Item::IronIngot);
    shape("minecraft:gold_sword", Item::GoldSword, 1, 1, 3, "mms", Item::GoldIngot);
    shape("minecraft:gold_pickaxe", Item::GoldPickaxe, 1, 3, 3, "mmm s  s ", Item::GoldIngot);
    shape("minecraft:gold_axe", Item::GoldAxe, 1, 2, 3, "mmms s", Item::GoldIngot);
    shape("minecraft:gold_shovel", Item::GoldShovel, 1, 1, 3, "mss", Item::GoldIngot);
    shape("minecraft:gold_hoe", Item::GoldHoe, 1, 2, 3, "mm s s", Item::GoldIngot);
    shape("minecraft:diamond_sword", Item::DiamondSword, 1, 1, 3, "mms", Item::Diamond);
    shape("minecraft:diamond_pickaxe", Item::DiamondPickaxe, 1, 3, 3, "mmm s  s ", Item::Diamond);
    shape("minecraft:diamond_axe", Item::DiamondAxe, 1, 2, 3, "mmms s", Item::Diamond);
    shape("minecraft:diamond_shovel", Item::DiamondShovel, 1, 1, 3, "mss", Item::Diamond);
    shape("minecraft:diamond_hoe", Item::DiamondHoe, 1, 2, 3, "mm s s", Item::Diamond);
    shape("minecraft:leather_helmet", Item::LeatherHelmet, 1, 3, 2, "mmmm m", Item::Leather);
    shape("minecraft:leather_chestplate", Item::LeatherChestplate, 1, 3, 3, "m mmmmmmm",
          Item::Leather);
    shape("minecraft:leather_leggings", Item::LeatherLeggings, 1, 3, 3, "mmmm mm m", Item::Leather);
    shape("minecraft:leather_boots", Item::LeatherBoots, 1, 3, 2, "m mm m", Item::Leather);
    shape("minecraft:iron_helmet", Item::IronHelmet, 1, 3, 2, "mmmm m", Item::IronIngot);
    shape("minecraft:iron_chestplate", Item::IronChestplate, 1, 3, 3, "m mmmmmmm", Item::IronIngot);
    shape("minecraft:iron_leggings", Item::IronLeggings, 1, 3, 3, "mmmm mm m", Item::IronIngot);
    shape("minecraft:iron_boots", Item::IronBoots, 1, 3, 2, "m mm m", Item::IronIngot);
    shape("minecraft:gold_helmet", Item::GoldHelmet, 1, 3, 2, "mmmm m", Item::GoldIngot);
    shape("minecraft:gold_chestplate", Item::GoldChestplate, 1, 3, 3, "m mmmmmmm", Item::GoldIngot);
    shape("minecraft:gold_leggings", Item::GoldLeggings, 1, 3, 3, "mmmm mm m", Item::GoldIngot);
    shape("minecraft:gold_boots", Item::GoldBoots, 1, 3, 2, "m mm m", Item::GoldIngot);
    shape("minecraft:diamond_helmet", Item::DiamondHelmet, 1, 3, 2, "mmmm m", Item::Diamond);
    shape("minecraft:diamond_chestplate", Item::DiamondChestplate, 1, 3, 3, "m mmmmmmm",
          Item::Diamond);
    shape("minecraft:diamond_leggings", Item::DiamondLeggings, 1, 3, 3, "mmmm mm m", Item::Diamond);
    shape("minecraft:diamond_boots", Item::DiamondBoots, 1, 3, 2, "m mm m", Item::Diamond);
    shape("minecraft:crafting_table", Item::CraftingTable, 1, 2, 2, "mmmm", Item::Planks);
    shape("minecraft:furnace", Item::Furnace, 1, 3, 3, "mmmm mmmm", Item::Cobblestone);
    shape("minecraft:chest", Item::Chest, 1, 3, 3, "mmmm mmmm", Item::Planks);
    shape("minecraft:bread", Item::Bread, 1, 3, 1, "mmm", Item::Wheat);
    add("minecraft:planks_from_SpruceLog", Item::SprucePlanks, 4, {{Item::SpruceLog, 1}});
    add("minecraft:planks_from_JungleLog", Item::JunglePlanks, 4, {{Item::JungleLog, 1}});
    add("minecraft:planks_from_AcaciaLog", Item::AcaciaPlanks, 4, {{Item::AcaciaLog, 1}});
    add("minecraft:planks_from_DarkOakLog", Item::DarkOakPlanks, 4, {{Item::DarkOakLog, 1}});
    add("minecraft:compress_Coal", Item::CoalBlock, 1, {{Item::Coal, 9}});
    add("minecraft:unpack_CoalBlock", Item::Coal, 9, {{Item::CoalBlock, 1}});
    add("minecraft:compress_IronIngot", Item::IronBlock, 1, {{Item::IronIngot, 9}});
    add("minecraft:unpack_IronBlock", Item::IronIngot, 9, {{Item::IronBlock, 1}});
    add("minecraft:compress_GoldIngot", Item::GoldBlock, 1, {{Item::GoldIngot, 9}});
    add("minecraft:unpack_GoldBlock", Item::GoldIngot, 9, {{Item::GoldBlock, 1}});
    add("minecraft:compress_Diamond", Item::DiamondBlock, 1, {{Item::Diamond, 9}});
    add("minecraft:unpack_DiamondBlock", Item::Diamond, 9, {{Item::DiamondBlock, 1}});
    add("minecraft:compress_Emerald", Item::EmeraldBlock, 1, {{Item::Emerald, 9}});
    add("minecraft:unpack_EmeraldBlock", Item::Emerald, 9, {{Item::EmeraldBlock, 1}});
    add("minecraft:compress_Lapis", Item::LapisBlock, 1, {{Item::Lapis, 9}});
    add("minecraft:unpack_LapisBlock", Item::Lapis, 9, {{Item::LapisBlock, 1}});
    add("minecraft:compress_Redstone", Item::RedstoneBlock, 1, {{Item::Redstone, 9}});
    add("minecraft:unpack_RedstoneBlock", Item::Redstone, 9, {{Item::RedstoneBlock, 1}});
    add("minecraft:compress_Quartz", Item::QuartzBlock, 1, {{Item::Quartz, 4}});
    add("minecraft:compress_Brick", Item::Bricks, 1, {{Item::Brick, 4}});
    add("minecraft:compress_NetherBrick", Item::NetherBricks, 1, {{Item::NetherBrick, 4}});
    add("minecraft:compress_GlowstoneDust", Item::Glowstone, 1, {{Item::GlowstoneDust, 4}});
    add("minecraft:shape_StoneBricks", Item::StoneBricks, 4, {{Item::Stone, 4}});
    add("minecraft:shape_PolishedGranite", Item::PolishedGranite, 4, {{Item::Granite, 4}});
    add("minecraft:shape_PolishedDiorite", Item::PolishedDiorite, 4, {{Item::Diorite, 4}});
    add("minecraft:shape_PolishedAndesite", Item::PolishedAndesite, 4, {{Item::Andesite, 4}});
    add("minecraft:shape_CutSandstone", Item::CutSandstone, 4, {{Item::Sandstone, 4}});
    add("minecraft:shape_CutRedSandstone", Item::CutRedSandstone, 4, {{Item::RedSandstone, 4}});
    add("minecraft:shape_EndStoneBricks", Item::EndStoneBricks, 4, {{Item::EndStone, 4}});
    add("minecraft:purpur_block", Item::Purpur, 4, {{Item::PoppedChorus, 4}}, Release::V1_9);
    add("minecraft:paper_book", Item::Book, 1, {{Item::Paper, 3}, {Item::Leather, 1}});
    add("minecraft:bookshelf", Item::Bookshelf, 1, {{Item::Planks, 6}, {Item::Book, 3}});
}
const Recipe* RecipeBook::match(const CraftGrid& grid, unsigned width, Release target) const {
    if (unsigned(target) < 100 || unsigned(target) > 109)
        return nullptr;
    for (auto item : grid)
        if (item > Item::Count)
            return nullptr;
    if (width != 2 && width != 3)
        return nullptr;
    for (unsigned i = width * width; i < 9; ++i)
        if (grid[i] != Item::Count)
            return nullptr;
    for (const auto& recipe : recipes_) {
        if (unsigned(recipe.since) > unsigned(target))
            continue;
        if (recipe.kind == RecipeKind::Shapeless) {
            std::array<unsigned, ItemCount> expected{}, actual{};
            for (const auto& ingredient : recipe.ingredients)
                if (ingredient.item < Item::Count)
                    expected[unsigned(ingredient.item)] += ingredient.count;
            for (unsigned i = 0; i < width * width; ++i)
                if (grid[i] < Item::Count)
                    ++actual[unsigned(grid[i])];
            if (actual == expected)
                return &recipe;
        } else if (recipe.kind == RecipeKind::Shaped && recipe.width <= width &&
                   recipe.height <= width) {
            for (unsigned y = 0; y <= width - recipe.height; ++y)
                for (unsigned x = 0; x <= width - recipe.width; ++x)
                    for (unsigned mirror = 0; mirror < 2; ++mirror) {
                        bool matches = true;
                        for (unsigned gy = 0; gy < width; ++gy)
                            for (unsigned gx = 0; gx < width; ++gx) {
                                Item expected = Item::Count;
                                if (gx >= x && gx < x + recipe.width && gy >= y &&
                                    gy < y + recipe.height) {
                                    const unsigned rx =
                                        mirror ? recipe.width - 1 - (gx - x) : gx - x;
                                    expected =
                                        recipe.ingredients[(gy - y) * recipe.width + rx].item;
                                }
                                if (grid[gy * width + gx] != expected)
                                    matches = false;
                            }
                        if (matches)
                            return &recipe;
                    }
        }
    }
    return nullptr;
}
bool RecipeBook::craft(const Recipe& recipe, Inventory& inventory) const {
    if (std::none_of(recipes_.begin(), recipes_.end(),
                     [&](const Recipe& r) { return &r == &recipe; }))
        return false;
    Inventory next = inventory;
    for (const auto& ingredient : recipe.ingredients)
        if (ingredient.item < Item::Count && !next.remove(ingredient.item, ingredient.count))
            return false;
    if (!next.add(recipe.result, recipe.resultCount))
        return false;
    inventory = next;
    return true;
}
Item smeltingResult(Item input) {
    switch (input) {
    case Item::IronOre:
        return Item::IronIngot;
    case Item::GoldOre:
        return Item::GoldIngot;
    case Item::Cobblestone:
        return Item::Stone;
    case Item::StoneBricks:
        return Item::CrackedStoneBricks;
    case Item::Netherrack:
        return Item::NetherBrick;
    case Item::OakLog:
    case Item::BirchLog:
    case Item::SpruceLog:
    case Item::JungleLog:
    case Item::AcaciaLog:
    case Item::DarkOakLog:
        return Item::Charcoal;
    case Item::Pork:
        return Item::CookedPork;
    case Item::Beef:
        return Item::Steak;
    case Item::Chicken:
        return Item::CookedChicken;
    case Item::Mutton:
        return Item::CookedMutton;
    case Item::Potato:
        return Item::BakedPotato;
    case Item::ChorusFruit:
        return Item::PoppedChorus;
    default:
        return Item::Count;
    }
}
float fuelSeconds(Item item) {
    switch (item) {
    case Item::Coal:
    case Item::Charcoal:
        return 80;
    case Item::CoalBlock:
        return 800;
    case Item::Stick:
        return 5;
    case Item::OakLog:
    case Item::BirchLog:
    case Item::SpruceLog:
    case Item::JungleLog:
    case Item::AcaciaLog:
    case Item::DarkOakLog:
    case Item::Planks:
    case Item::BirchPlanks:
    case Item::SprucePlanks:
    case Item::JunglePlanks:
    case Item::AcaciaPlanks:
    case Item::DarkOakPlanks:
    case Item::CraftingTable:
    case Item::Chest:
        return 15;
    default:
        return 0;
    }
}
bool Furnace::deposit(Inventory& inventory, Item item, bool asFuel, bool wholeStack) {
    if (item == Item::Count ||
        (asFuel ? fuelSeconds(item) == 0 : smeltingResult(item) == Item::Count))
        return false;
    auto& type = asFuel ? fuel : input;
    auto& count = asFuel ? fuelCount : inputCount;
    if (count && type != item)
        return false;
    const unsigned amount = std::min({Inventory::StackLimit - count, inventory.count(item),
                                      wholeStack ? Inventory::StackLimit : 1u});
    if (!amount)
        return false;
    inventory.remove(item, amount);
    type = item;
    count += amount;
    return true;
}
bool Furnace::take(Inventory& inventory, unsigned slot) {
    if (slot > 2)
        return false;
    auto& type = slot == 0 ? input : slot == 1 ? fuel : output;
    auto& count = slot == 0 ? inputCount : slot == 1 ? fuelCount : outputCount;
    const auto amount = std::min(count, Inventory::StackLimit - inventory.count(type));
    if (!amount || !inventory.add(type, amount))
        return false;
    count -= amount;
    if (!count) {
        type = Item::Count;
        if (slot == 0)
            progress = 0;
    }
    return true;
}
void Furnace::tick(float seconds) {
    if (!std::isfinite(seconds) || seconds <= 0)
        return;
    seconds = std::min(seconds, 60.f); // Bounded catch-up after suspend.
    while (seconds > .00001f) {
        const auto result = smeltingResult(input);
        const bool ready = inputCount && result != Item::Count &&
                           outputCount < Inventory::StackLimit &&
                           (!outputCount || output == result);
        if (burn <= 0 && ready && fuelCount && fuelSeconds(fuel) > 0) {
            burn = fuelSeconds(fuel);
            if (!--fuelCount)
                fuel = Item::Count;
        }
        if (burn <= 0 || !ready) {
            burn = std::max(0.f, burn - seconds);
            progress = std::max(0.f, progress - 2 * seconds);
            return;
        }
        const float step = std::min({seconds, burn, 10 - progress});
        burn -= step;
        progress += step;
        seconds -= step;
        if (progress >= 9.99999f) {
            output = result;
            ++outputCount;
            if (!--inputCount)
                input = Item::Count;
            progress = 0;
        }
    }
}
} // namespace voxel

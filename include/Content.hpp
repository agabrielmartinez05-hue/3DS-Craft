#pragma once
#include <array>
#include <cstdint>

namespace voxel {
// Append stable IDs; never reorder them (save format uses these values).
enum class Block : std::uint16_t {
    Air,
    Grass,
    Dirt,
    Stone,
    OakLog,
    BirchLog,
    OakLeaves,
    BirchLeaves,
    Planks,
    Netherrack,
    EndStone,
    Obsidian,
    IronOre,
    Lava,
    Cobblestone,
    Path,
    Chest,
    CoalOre,
    GoldOre,
    DiamondOre,
    EmeraldOre,
    LapisOre,
    RedstoneOre,
    QuartzOre,
    Granite,
    Diorite,
    Andesite,
    PolishedGranite,
    PolishedDiorite,
    PolishedAndesite,
    Sandstone,
    ChiseledSandstone,
    CutSandstone,
    RedSandstone,
    ChiseledRedSandstone,
    CutRedSandstone,
    StoneBricks,
    MossyStoneBricks,
    CrackedStoneBricks,
    ChiseledStoneBricks,
    MossyCobblestone,
    Bricks,
    SpruceLog,
    JungleLog,
    AcaciaLog,
    DarkOakLog,
    SprucePlanks,
    BirchPlanks,
    JunglePlanks,
    AcaciaPlanks,
    DarkOakPlanks,
    CoalBlock,
    IronBlock,
    GoldBlock,
    DiamondBlock,
    EmeraldBlock,
    LapisBlock,
    RedstoneBlock,
    QuartzBlock,
    QuartzPillar,
    ChiseledQuartz,
    NetherBricks,
    Prismarine,
    PrismarineBricks,
    DarkPrismarine,
    SeaLantern,
    Glowstone,
    Purpur,
    PurpurPillar,
    EndStoneBricks,
    CraftingTable,
    Furnace,
    Bookshelf,
    Terracotta,
    WhiteWool,
    OrangeWool,
    MagentaWool,
    LightBlueWool,
    YellowWool,
    LimeWool,
    PinkWool,
    GrayWool,
    LightGrayWool,
    CyanWool,
    PurpleWool,
    BlueWool,
    BrownWool,
    GreenWool,
    RedWool,
    BlackWool,
    WhiteTerracotta,
    OrangeTerracotta,
    MagentaTerracotta,
    LightBlueTerracotta,
    YellowTerracotta,
    LimeTerracotta,
    PinkTerracotta,
    GrayTerracotta,
    LightGrayTerracotta,
    CyanTerracotta,
    PurpleTerracotta,
    BlueTerracotta,
    BrownTerracotta,
    GreenTerracotta,
    RedTerracotta,
    BlackTerracotta,
    Count
};
enum class Item : std::uint16_t {
    Grass,
    Dirt,
    Stone,
    Wool,
    Pork,
    Leather,
    OakLog,
    BirchLog,
    OakLeaves,
    BirchLeaves,
    Planks,
    Stick,
    Netherrack,
    EndStone,
    Obsidian,
    IronOre,
    IronIngot,
    Sword,
    Pickaxe,
    Shield,
    Elytra,
    Cobblestone,
    Path,
    Chest,
    CoalOre,
    GoldOre,
    DiamondOre,
    EmeraldOre,
    LapisOre,
    RedstoneOre,
    QuartzOre,
    Granite,
    Diorite,
    Andesite,
    PolishedGranite,
    PolishedDiorite,
    PolishedAndesite,
    Sandstone,
    ChiseledSandstone,
    CutSandstone,
    RedSandstone,
    ChiseledRedSandstone,
    CutRedSandstone,
    StoneBricks,
    MossyStoneBricks,
    CrackedStoneBricks,
    ChiseledStoneBricks,
    MossyCobblestone,
    Bricks,
    SpruceLog,
    JungleLog,
    AcaciaLog,
    DarkOakLog,
    SprucePlanks,
    BirchPlanks,
    JunglePlanks,
    AcaciaPlanks,
    DarkOakPlanks,
    CoalBlock,
    IronBlock,
    GoldBlock,
    DiamondBlock,
    EmeraldBlock,
    LapisBlock,
    RedstoneBlock,
    QuartzBlock,
    QuartzPillar,
    ChiseledQuartz,
    NetherBricks,
    Prismarine,
    PrismarineBricks,
    DarkPrismarine,
    SeaLantern,
    Glowstone,
    Purpur,
    PurpurPillar,
    EndStoneBricks,
    CraftingTable,
    Furnace,
    Bookshelf,
    Terracotta,
    OrangeWool,
    MagentaWool,
    LightBlueWool,
    YellowWool,
    LimeWool,
    PinkWool,
    GrayWool,
    LightGrayWool,
    CyanWool,
    PurpleWool,
    BlueWool,
    BrownWool,
    GreenWool,
    RedWool,
    BlackWool,
    WhiteTerracotta,
    OrangeTerracotta,
    MagentaTerracotta,
    LightBlueTerracotta,
    YellowTerracotta,
    LimeTerracotta,
    PinkTerracotta,
    GrayTerracotta,
    LightGrayTerracotta,
    CyanTerracotta,
    PurpleTerracotta,
    BlueTerracotta,
    BrownTerracotta,
    GreenTerracotta,
    RedTerracotta,
    BlackTerracotta,
    Coal,
    Charcoal,
    GoldIngot,
    Diamond,
    Emerald,
    Lapis,
    Redstone,
    Quartz,
    Brick,
    NetherBrick,
    GlowstoneDust,
    PrismarineShard,
    PrismarineCrystal,
    ChorusFruit,
    PoppedChorus,
    Paper,
    Book,
    Sugar,
    Wheat,
    Egg,
    Apple,
    Bread,
    CookedPork,
    Beef,
    Steak,
    Chicken,
    CookedChicken,
    Mutton,
    CookedMutton,
    Carrot,
    Potato,
    BakedPotato,
    Beetroot,
    Cookie,
    Melon,
    PumpkinPie,
    WoodSword,
    WoodPickaxe,
    WoodAxe,
    WoodShovel,
    WoodHoe,
    StoneSword,
    StonePickaxe,
    StoneAxe,
    StoneShovel,
    StoneHoe,
    IronAxe,
    IronShovel,
    IronHoe,
    GoldSword,
    GoldPickaxe,
    GoldAxe,
    GoldShovel,
    GoldHoe,
    DiamondSword,
    DiamondPickaxe,
    DiamondAxe,
    DiamondShovel,
    DiamondHoe,
    LeatherHelmet,
    LeatherChestplate,
    LeatherLeggings,
    LeatherBoots,
    ChainHelmet,
    ChainChestplate,
    ChainLeggings,
    ChainBoots,
    IronHelmet,
    IronChestplate,
    IronLeggings,
    IronBoots,
    GoldHelmet,
    GoldChestplate,
    GoldLeggings,
    GoldBoots,
    DiamondHelmet,
    DiamondChestplate,
    DiamondLeggings,
    DiamondBoots,
    Count
};
constexpr unsigned BlockCount = unsigned(Block::Count);
constexpr unsigned ItemCount = unsigned(Item::Count);
constexpr std::array<Item, 12> HotbarItems{
    {Item::Grass, Item::Dirt, Item::Stone, Item::OakLog, Item::BirchLog, Item::OakLeaves,
     Item::BirchLeaves, Item::Planks, Item::Sword, Item::Pickaxe, Item::Pork, Item::Shield}};
enum class Release : unsigned {
    V1_0 = 100,
    V1_1 = 101,
    V1_2 = 102,
    V1_3 = 103,
    V1_4 = 104,
    V1_5 = 105,
    V1_6 = 106,
    V1_7 = 107,
    V1_8 = 108,
    V1_9 = 109
};
enum class RenderLayer { Opaque, Cutout, Translucent };
struct BlockTraits {
    const char* id;
    Item drop;
    RenderLayer layer;
    float hardness;
    Release since;
};
inline constexpr std::array<BlockTraits, BlockCount> BlockTypes{
    {{"air", Item::Count, RenderLayer::Opaque, 0, Release::V1_0},
     {"grass_block", Item::Grass, RenderLayer::Opaque, 0.6f, Release::V1_0},
     {"dirt", Item::Dirt, RenderLayer::Opaque, 0.5f, Release::V1_0},
     {"stone", Item::Cobblestone, RenderLayer::Opaque, 1.5f, Release::V1_0},
     {"oak_log", Item::OakLog, RenderLayer::Opaque, 2, Release::V1_0},
     {"birch_log", Item::BirchLog, RenderLayer::Opaque, 2, Release::V1_0},
     {"oak_leaves", Item::OakLeaves, RenderLayer::Cutout, 0.2f, Release::V1_0},
     {"birch_leaves", Item::BirchLeaves, RenderLayer::Cutout, 0.2f, Release::V1_0},
     {"oak_planks", Item::Planks, RenderLayer::Opaque, 2, Release::V1_0},
     {"netherrack", Item::Netherrack, RenderLayer::Opaque, 0.4f, Release::V1_0},
     {"end_stone", Item::EndStone, RenderLayer::Opaque, 3, Release::V1_0},
     {"obsidian", Item::Obsidian, RenderLayer::Opaque, 8, Release::V1_0},
     {"iron_ore", Item::IronOre, RenderLayer::Opaque, 3, Release::V1_0},
     {"lava", Item::Count, RenderLayer::Opaque, 0, Release::V1_0},
     {"cobblestone", Item::Cobblestone, RenderLayer::Opaque, 2, Release::V1_0},
     {"dirt_path", Item::Dirt, RenderLayer::Opaque, .65f, Release::V1_9},
     {"chest", Item::Chest, RenderLayer::Opaque, 2.5f, Release::V1_0},
     {"coal_ore", Item::Coal, RenderLayer::Opaque, 3.0f, Release::V1_0},
     {"gold_ore", Item::GoldOre, RenderLayer::Opaque, 3.0f, Release::V1_0},
     {"diamond_ore", Item::Diamond, RenderLayer::Opaque, 3.0f, Release::V1_0},
     {"emerald_ore", Item::Emerald, RenderLayer::Opaque, 3.0f, Release::V1_3},
     {"lapis_ore", Item::Lapis, RenderLayer::Opaque, 3.0f, Release::V1_0},
     {"redstone_ore", Item::Redstone, RenderLayer::Opaque, 3.0f, Release::V1_0},
     {"nether_quartz_ore", Item::Quartz, RenderLayer::Opaque, 3.0f, Release::V1_5},
     {"granite", Item::Granite, RenderLayer::Opaque, 1.5f, Release::V1_8},
     {"diorite", Item::Diorite, RenderLayer::Opaque, 1.5f, Release::V1_8},
     {"andesite", Item::Andesite, RenderLayer::Opaque, 1.5f, Release::V1_8},
     {"polished_granite", Item::PolishedGranite, RenderLayer::Opaque, 1.5f, Release::V1_8},
     {"polished_diorite", Item::PolishedDiorite, RenderLayer::Opaque, 1.5f, Release::V1_8},
     {"polished_andesite", Item::PolishedAndesite, RenderLayer::Opaque, 1.5f, Release::V1_8},
     {"sandstone", Item::Sandstone, RenderLayer::Opaque, 0.8f, Release::V1_0},
     {"chiseled_sandstone", Item::ChiseledSandstone, RenderLayer::Opaque, 0.8f, Release::V1_2},
     {"cut_sandstone", Item::CutSandstone, RenderLayer::Opaque, 0.8f, Release::V1_0},
     {"red_sandstone", Item::RedSandstone, RenderLayer::Opaque, 0.8f, Release::V1_8},
     {"chiseled_red_sandstone", Item::ChiseledRedSandstone, RenderLayer::Opaque, 0.8f,
      Release::V1_8},
     {"cut_red_sandstone", Item::CutRedSandstone, RenderLayer::Opaque, 0.8f, Release::V1_8},
     {"stone_bricks", Item::StoneBricks, RenderLayer::Opaque, 1.5f, Release::V1_0},
     {"mossy_stone_bricks", Item::MossyStoneBricks, RenderLayer::Opaque, 1.5f, Release::V1_0},
     {"cracked_stone_bricks", Item::CrackedStoneBricks, RenderLayer::Opaque, 1.5f, Release::V1_0},
     {"chiseled_stone_bricks", Item::ChiseledStoneBricks, RenderLayer::Opaque, 1.5f, Release::V1_2},
     {"mossy_cobblestone", Item::MossyCobblestone, RenderLayer::Opaque, 2.0f, Release::V1_0},
     {"bricks", Item::Bricks, RenderLayer::Opaque, 2.0f, Release::V1_0},
     {"spruce_log", Item::SpruceLog, RenderLayer::Opaque, 2.0f, Release::V1_0},
     {"jungle_log", Item::JungleLog, RenderLayer::Opaque, 2.0f, Release::V1_2},
     {"acacia_log", Item::AcaciaLog, RenderLayer::Opaque, 2.0f, Release::V1_7},
     {"dark_oak_log", Item::DarkOakLog, RenderLayer::Opaque, 2.0f, Release::V1_7},
     {"spruce_planks", Item::SprucePlanks, RenderLayer::Opaque, 2.0f, Release::V1_2},
     {"birch_planks", Item::BirchPlanks, RenderLayer::Opaque, 2.0f, Release::V1_2},
     {"jungle_planks", Item::JunglePlanks, RenderLayer::Opaque, 2.0f, Release::V1_2},
     {"acacia_planks", Item::AcaciaPlanks, RenderLayer::Opaque, 2.0f, Release::V1_7},
     {"dark_oak_planks", Item::DarkOakPlanks, RenderLayer::Opaque, 2.0f, Release::V1_7},
     {"coal_block", Item::CoalBlock, RenderLayer::Opaque, 5.0f, Release::V1_6},
     {"iron_block", Item::IronBlock, RenderLayer::Opaque, 5.0f, Release::V1_0},
     {"gold_block", Item::GoldBlock, RenderLayer::Opaque, 3.0f, Release::V1_0},
     {"diamond_block", Item::DiamondBlock, RenderLayer::Opaque, 5.0f, Release::V1_0},
     {"emerald_block", Item::EmeraldBlock, RenderLayer::Opaque, 5.0f, Release::V1_3},
     {"lapis_block", Item::LapisBlock, RenderLayer::Opaque, 3.0f, Release::V1_0},
     {"redstone_block", Item::RedstoneBlock, RenderLayer::Opaque, 5.0f, Release::V1_5},
     {"quartz_block", Item::QuartzBlock, RenderLayer::Opaque, 0.8f, Release::V1_5},
     {"quartz_pillar", Item::QuartzPillar, RenderLayer::Opaque, 0.8f, Release::V1_5},
     {"chiseled_quartz_block", Item::ChiseledQuartz, RenderLayer::Opaque, 0.8f, Release::V1_5},
     {"nether_bricks", Item::NetherBricks, RenderLayer::Opaque, 2.0f, Release::V1_0},
     {"prismarine", Item::Prismarine, RenderLayer::Opaque, 1.5f, Release::V1_8},
     {"prismarine_bricks", Item::PrismarineBricks, RenderLayer::Opaque, 1.5f, Release::V1_8},
     {"dark_prismarine", Item::DarkPrismarine, RenderLayer::Opaque, 1.5f, Release::V1_8},
     {"sea_lantern", Item::SeaLantern, RenderLayer::Opaque, 0.3f, Release::V1_8},
     {"glowstone", Item::GlowstoneDust, RenderLayer::Opaque, 0.3f, Release::V1_0},
     {"purpur_block", Item::Purpur, RenderLayer::Opaque, 1.5f, Release::V1_9},
     {"purpur_pillar", Item::PurpurPillar, RenderLayer::Opaque, 1.5f, Release::V1_9},
     {"end_stone_bricks", Item::EndStoneBricks, RenderLayer::Opaque, 0.8f, Release::V1_9},
     {"crafting_table", Item::CraftingTable, RenderLayer::Opaque, 2.5f, Release::V1_0},
     {"furnace", Item::Furnace, RenderLayer::Opaque, 3.5f, Release::V1_0},
     {"bookshelf", Item::Bookshelf, RenderLayer::Opaque, 1.5f, Release::V1_0},
     {"terracotta", Item::Terracotta, RenderLayer::Opaque, 1.25f, Release::V1_6},
     {"white_wool", Item::Wool, RenderLayer::Opaque, 0.8f, Release::V1_0},
     {"orange_wool", Item::OrangeWool, RenderLayer::Opaque, 0.8f, Release::V1_0},
     {"magenta_wool", Item::MagentaWool, RenderLayer::Opaque, 0.8f, Release::V1_0},
     {"light_blue_wool", Item::LightBlueWool, RenderLayer::Opaque, 0.8f, Release::V1_0},
     {"yellow_wool", Item::YellowWool, RenderLayer::Opaque, 0.8f, Release::V1_0},
     {"lime_wool", Item::LimeWool, RenderLayer::Opaque, 0.8f, Release::V1_0},
     {"pink_wool", Item::PinkWool, RenderLayer::Opaque, 0.8f, Release::V1_0},
     {"gray_wool", Item::GrayWool, RenderLayer::Opaque, 0.8f, Release::V1_0},
     {"light_gray_wool", Item::LightGrayWool, RenderLayer::Opaque, 0.8f, Release::V1_0},
     {"cyan_wool", Item::CyanWool, RenderLayer::Opaque, 0.8f, Release::V1_0},
     {"purple_wool", Item::PurpleWool, RenderLayer::Opaque, 0.8f, Release::V1_0},
     {"blue_wool", Item::BlueWool, RenderLayer::Opaque, 0.8f, Release::V1_0},
     {"brown_wool", Item::BrownWool, RenderLayer::Opaque, 0.8f, Release::V1_0},
     {"green_wool", Item::GreenWool, RenderLayer::Opaque, 0.8f, Release::V1_0},
     {"red_wool", Item::RedWool, RenderLayer::Opaque, 0.8f, Release::V1_0},
     {"black_wool", Item::BlackWool, RenderLayer::Opaque, 0.8f, Release::V1_0},
     {"white_terracotta", Item::WhiteTerracotta, RenderLayer::Opaque, 1.25f, Release::V1_0},
     {"orange_terracotta", Item::OrangeTerracotta, RenderLayer::Opaque, 1.25f, Release::V1_0},
     {"magenta_terracotta", Item::MagentaTerracotta, RenderLayer::Opaque, 1.25f, Release::V1_0},
     {"light_blue_terracotta", Item::LightBlueTerracotta, RenderLayer::Opaque, 1.25f,
      Release::V1_0},
     {"yellow_terracotta", Item::YellowTerracotta, RenderLayer::Opaque, 1.25f, Release::V1_0},
     {"lime_terracotta", Item::LimeTerracotta, RenderLayer::Opaque, 1.25f, Release::V1_0},
     {"pink_terracotta", Item::PinkTerracotta, RenderLayer::Opaque, 1.25f, Release::V1_0},
     {"gray_terracotta", Item::GrayTerracotta, RenderLayer::Opaque, 1.25f, Release::V1_0},
     {"light_gray_terracotta", Item::LightGrayTerracotta, RenderLayer::Opaque, 1.25f,
      Release::V1_0},
     {"cyan_terracotta", Item::CyanTerracotta, RenderLayer::Opaque, 1.25f, Release::V1_0},
     {"purple_terracotta", Item::PurpleTerracotta, RenderLayer::Opaque, 1.25f, Release::V1_0},
     {"blue_terracotta", Item::BlueTerracotta, RenderLayer::Opaque, 1.25f, Release::V1_0},
     {"brown_terracotta", Item::BrownTerracotta, RenderLayer::Opaque, 1.25f, Release::V1_0},
     {"green_terracotta", Item::GreenTerracotta, RenderLayer::Opaque, 1.25f, Release::V1_0},
     {"red_terracotta", Item::RedTerracotta, RenderLayer::Opaque, 1.25f, Release::V1_0},
     {"black_terracotta", Item::BlackTerracotta, RenderLayer::Opaque, 1.25f, Release::V1_0}}};
struct ItemTraits {
    const char* id;
    const char* name;
    Block places;
    Release since;
};
inline constexpr std::array<ItemTraits, ItemCount> ItemTypes{
    {{"grass_block", "Grass", Block::Grass, Release::V1_0},
     {"dirt", "Dirt", Block::Dirt, Release::V1_0},
     {"stone", "Stone", Block::Stone, Release::V1_0},
     {"white_wool", "Wool", Block::WhiteWool, Release::V1_0},
     {"porkchop", "Pork", Block::Air, Release::V1_0},
     {"leather", "Leather", Block::Air, Release::V1_0},
     {"oak_log", "Oak log", Block::OakLog, Release::V1_0},
     {"birch_log", "Birch log", Block::BirchLog, Release::V1_0},
     {"oak_leaves", "Oak leaves", Block::OakLeaves, Release::V1_0},
     {"birch_leaves", "Birch leaves", Block::BirchLeaves, Release::V1_0},
     {"oak_planks", "Planks", Block::Planks, Release::V1_0},
     {"stick", "Stick", Block::Air, Release::V1_0},
     {"netherrack", "Netherrack", Block::Netherrack, Release::V1_0},
     {"end_stone", "End stone", Block::EndStone, Release::V1_0},
     {"obsidian", "Obsidian", Block::Obsidian, Release::V1_0},
     {"iron_ore", "Iron ore", Block::IronOre, Release::V1_0},
     {"iron_ingot", "Iron ingot", Block::Air, Release::V1_0},
     {"iron_sword", "Sword", Block::Air, Release::V1_0},
     {"iron_pickaxe", "Pickaxe", Block::Air, Release::V1_0},
     {"shield", "Shield", Block::Air, Release::V1_9},
     {"elytra", "Elytra", Block::Air, Release::V1_9},
     {"cobblestone", "Cobblestone", Block::Cobblestone, Release::V1_0},
     {"dirt_path", "Path", Block::Path, Release::V1_9},
     {"chest", "Chest", Block::Chest, Release::V1_0},
     {"coal_ore", "Coal Ore", Block::CoalOre, Release::V1_0},
     {"gold_ore", "Gold Ore", Block::GoldOre, Release::V1_0},
     {"diamond_ore", "Diamond Ore", Block::DiamondOre, Release::V1_0},
     {"emerald_ore", "Emerald Ore", Block::EmeraldOre, Release::V1_3},
     {"lapis_ore", "Lapis Ore", Block::LapisOre, Release::V1_0},
     {"redstone_ore", "Redstone Ore", Block::RedstoneOre, Release::V1_0},
     {"nether_quartz_ore", "Quartz Ore", Block::QuartzOre, Release::V1_5},
     {"granite", "Granite", Block::Granite, Release::V1_8},
     {"diorite", "Diorite", Block::Diorite, Release::V1_8},
     {"andesite", "Andesite", Block::Andesite, Release::V1_8},
     {"polished_granite", "Polished Granite", Block::PolishedGranite, Release::V1_8},
     {"polished_diorite", "Polished Diorite", Block::PolishedDiorite, Release::V1_8},
     {"polished_andesite", "Polished Andesite", Block::PolishedAndesite, Release::V1_8},
     {"sandstone", "Sandstone", Block::Sandstone, Release::V1_0},
     {"chiseled_sandstone", "Chiseled Sandstone", Block::ChiseledSandstone, Release::V1_2},
     {"cut_sandstone", "Cut Sandstone", Block::CutSandstone, Release::V1_0},
     {"red_sandstone", "Red Sandstone", Block::RedSandstone, Release::V1_8},
     {"chiseled_red_sandstone", "Chiseled Red Sandstone", Block::ChiseledRedSandstone,
      Release::V1_8},
     {"cut_red_sandstone", "Cut Red Sandstone", Block::CutRedSandstone, Release::V1_8},
     {"stone_bricks", "Stone Bricks", Block::StoneBricks, Release::V1_0},
     {"mossy_stone_bricks", "Mossy Stone Bricks", Block::MossyStoneBricks, Release::V1_0},
     {"cracked_stone_bricks", "Cracked Stone Bricks", Block::CrackedStoneBricks, Release::V1_0},
     {"chiseled_stone_bricks", "Chiseled Stone Bricks", Block::ChiseledStoneBricks, Release::V1_2},
     {"mossy_cobblestone", "Mossy Cobblestone", Block::MossyCobblestone, Release::V1_0},
     {"bricks", "Bricks", Block::Bricks, Release::V1_0},
     {"spruce_log", "Spruce Log", Block::SpruceLog, Release::V1_0},
     {"jungle_log", "Jungle Log", Block::JungleLog, Release::V1_2},
     {"acacia_log", "Acacia Log", Block::AcaciaLog, Release::V1_7},
     {"dark_oak_log", "Dark Oak Log", Block::DarkOakLog, Release::V1_7},
     {"spruce_planks", "Spruce Planks", Block::SprucePlanks, Release::V1_2},
     {"birch_planks", "Birch Planks", Block::BirchPlanks, Release::V1_2},
     {"jungle_planks", "Jungle Planks", Block::JunglePlanks, Release::V1_2},
     {"acacia_planks", "Acacia Planks", Block::AcaciaPlanks, Release::V1_7},
     {"dark_oak_planks", "Dark Oak Planks", Block::DarkOakPlanks, Release::V1_7},
     {"coal_block", "Coal Block", Block::CoalBlock, Release::V1_6},
     {"iron_block", "Iron Block", Block::IronBlock, Release::V1_0},
     {"gold_block", "Gold Block", Block::GoldBlock, Release::V1_0},
     {"diamond_block", "Diamond Block", Block::DiamondBlock, Release::V1_0},
     {"emerald_block", "Emerald Block", Block::EmeraldBlock, Release::V1_3},
     {"lapis_block", "Lapis Block", Block::LapisBlock, Release::V1_0},
     {"redstone_block", "Redstone Block", Block::RedstoneBlock, Release::V1_5},
     {"quartz_block", "Quartz Block", Block::QuartzBlock, Release::V1_5},
     {"quartz_pillar", "Quartz Pillar", Block::QuartzPillar, Release::V1_5},
     {"chiseled_quartz_block", "Chiseled Quartz", Block::ChiseledQuartz, Release::V1_5},
     {"nether_bricks", "Nether Bricks", Block::NetherBricks, Release::V1_0},
     {"prismarine", "Prismarine", Block::Prismarine, Release::V1_8},
     {"prismarine_bricks", "Prismarine Bricks", Block::PrismarineBricks, Release::V1_8},
     {"dark_prismarine", "Dark Prismarine", Block::DarkPrismarine, Release::V1_8},
     {"sea_lantern", "Sea Lantern", Block::SeaLantern, Release::V1_8},
     {"glowstone", "Glowstone", Block::Glowstone, Release::V1_0},
     {"purpur_block", "Purpur", Block::Purpur, Release::V1_9},
     {"purpur_pillar", "Purpur Pillar", Block::PurpurPillar, Release::V1_9},
     {"end_stone_bricks", "End Stone Bricks", Block::EndStoneBricks, Release::V1_9},
     {"crafting_table", "Crafting Table", Block::CraftingTable, Release::V1_0},
     {"furnace", "Furnace", Block::Furnace, Release::V1_0},
     {"bookshelf", "Bookshelf", Block::Bookshelf, Release::V1_0},
     {"terracotta", "Terracotta", Block::Terracotta, Release::V1_6},
     {"orange_wool", "Orange Wool", Block::OrangeWool, Release::V1_0},
     {"magenta_wool", "Magenta Wool", Block::MagentaWool, Release::V1_0},
     {"light_blue_wool", "Light Blue Wool", Block::LightBlueWool, Release::V1_0},
     {"yellow_wool", "Yellow Wool", Block::YellowWool, Release::V1_0},
     {"lime_wool", "Lime Wool", Block::LimeWool, Release::V1_0},
     {"pink_wool", "Pink Wool", Block::PinkWool, Release::V1_0},
     {"gray_wool", "Gray Wool", Block::GrayWool, Release::V1_0},
     {"light_gray_wool", "Light Gray Wool", Block::LightGrayWool, Release::V1_0},
     {"cyan_wool", "Cyan Wool", Block::CyanWool, Release::V1_0},
     {"purple_wool", "Purple Wool", Block::PurpleWool, Release::V1_0},
     {"blue_wool", "Blue Wool", Block::BlueWool, Release::V1_0},
     {"brown_wool", "Brown Wool", Block::BrownWool, Release::V1_0},
     {"green_wool", "Green Wool", Block::GreenWool, Release::V1_0},
     {"red_wool", "Red Wool", Block::RedWool, Release::V1_0},
     {"black_wool", "Black Wool", Block::BlackWool, Release::V1_0},
     {"white_terracotta", "White Terracotta", Block::WhiteTerracotta, Release::V1_0},
     {"orange_terracotta", "Orange Terracotta", Block::OrangeTerracotta, Release::V1_0},
     {"magenta_terracotta", "Magenta Terracotta", Block::MagentaTerracotta, Release::V1_0},
     {"light_blue_terracotta", "Light Blue Terracotta", Block::LightBlueTerracotta, Release::V1_0},
     {"yellow_terracotta", "Yellow Terracotta", Block::YellowTerracotta, Release::V1_0},
     {"lime_terracotta", "Lime Terracotta", Block::LimeTerracotta, Release::V1_0},
     {"pink_terracotta", "Pink Terracotta", Block::PinkTerracotta, Release::V1_0},
     {"gray_terracotta", "Gray Terracotta", Block::GrayTerracotta, Release::V1_0},
     {"light_gray_terracotta", "Light Gray Terracotta", Block::LightGrayTerracotta, Release::V1_0},
     {"cyan_terracotta", "Cyan Terracotta", Block::CyanTerracotta, Release::V1_0},
     {"purple_terracotta", "Purple Terracotta", Block::PurpleTerracotta, Release::V1_0},
     {"blue_terracotta", "Blue Terracotta", Block::BlueTerracotta, Release::V1_0},
     {"brown_terracotta", "Brown Terracotta", Block::BrownTerracotta, Release::V1_0},
     {"green_terracotta", "Green Terracotta", Block::GreenTerracotta, Release::V1_0},
     {"red_terracotta", "Red Terracotta", Block::RedTerracotta, Release::V1_0},
     {"black_terracotta", "Black Terracotta", Block::BlackTerracotta, Release::V1_0},
     {"coal", "Coal", Block::Air, Release::V1_0},
     {"charcoal", "Charcoal", Block::Air, Release::V1_0},
     {"gold_ingot", "Gold Ingot", Block::Air, Release::V1_0},
     {"diamond", "Diamond", Block::Air, Release::V1_0},
     {"emerald", "Emerald", Block::Air, Release::V1_0},
     {"lapis_lazuli", "Lapis", Block::Air, Release::V1_0},
     {"redstone", "Redstone", Block::Air, Release::V1_0},
     {"quartz", "Quartz", Block::Air, Release::V1_0},
     {"brick", "Brick", Block::Air, Release::V1_0},
     {"nether_brick", "Nether Brick", Block::Air, Release::V1_0},
     {"glowstone_dust", "Glowstone Dust", Block::Air, Release::V1_0},
     {"prismarine_shard", "Prismarine Shard", Block::Air, Release::V1_0},
     {"prismarine_crystals", "Prismarine Crystal", Block::Air, Release::V1_0},
     {"chorus_fruit", "Chorus Fruit", Block::Air, Release::V1_9},
     {"popped_chorus_fruit", "Popped Chorus", Block::Air, Release::V1_9},
     {"paper", "Paper", Block::Air, Release::V1_0},
     {"book", "Book", Block::Air, Release::V1_0},
     {"sugar", "Sugar", Block::Air, Release::V1_0},
     {"wheat", "Wheat", Block::Air, Release::V1_0},
     {"egg", "Egg", Block::Air, Release::V1_0},
     {"apple", "Apple", Block::Air, Release::V1_0},
     {"bread", "Bread", Block::Air, Release::V1_0},
     {"cooked_porkchop", "Cooked Pork", Block::Air, Release::V1_0},
     {"beef", "Beef", Block::Air, Release::V1_0},
     {"cooked_beef", "Steak", Block::Air, Release::V1_0},
     {"chicken", "Chicken", Block::Air, Release::V1_0},
     {"cooked_chicken", "Cooked Chicken", Block::Air, Release::V1_0},
     {"mutton", "Mutton", Block::Air, Release::V1_0},
     {"cooked_mutton", "Cooked Mutton", Block::Air, Release::V1_0},
     {"carrot", "Carrot", Block::Air, Release::V1_0},
     {"potato", "Potato", Block::Air, Release::V1_0},
     {"baked_potato", "Baked Potato", Block::Air, Release::V1_0},
     {"beetroot", "Beetroot", Block::Air, Release::V1_9},
     {"cookie", "Cookie", Block::Air, Release::V1_0},
     {"melon_slice", "Melon", Block::Air, Release::V1_0},
     {"pumpkin_pie", "Pumpkin Pie", Block::Air, Release::V1_0},
     {"wooden_sword", "Wood Sword", Block::Air, Release::V1_0},
     {"wooden_pickaxe", "Wood Pickaxe", Block::Air, Release::V1_0},
     {"wooden_axe", "Wood Axe", Block::Air, Release::V1_0},
     {"wooden_shovel", "Wood Shovel", Block::Air, Release::V1_0},
     {"wooden_hoe", "Wood Hoe", Block::Air, Release::V1_0},
     {"stone_sword", "Stone Sword", Block::Air, Release::V1_0},
     {"stone_pickaxe", "Stone Pickaxe", Block::Air, Release::V1_0},
     {"stone_axe", "Stone Axe", Block::Air, Release::V1_0},
     {"stone_shovel", "Stone Shovel", Block::Air, Release::V1_0},
     {"stone_hoe", "Stone Hoe", Block::Air, Release::V1_0},
     {"iron_axe", "Iron Axe", Block::Air, Release::V1_0},
     {"iron_shovel", "Iron Shovel", Block::Air, Release::V1_0},
     {"iron_hoe", "Iron Hoe", Block::Air, Release::V1_0},
     {"golden_sword", "Gold Sword", Block::Air, Release::V1_0},
     {"golden_pickaxe", "Gold Pickaxe", Block::Air, Release::V1_0},
     {"golden_axe", "Gold Axe", Block::Air, Release::V1_0},
     {"golden_shovel", "Gold Shovel", Block::Air, Release::V1_0},
     {"golden_hoe", "Gold Hoe", Block::Air, Release::V1_0},
     {"diamond_sword", "Diamond Sword", Block::Air, Release::V1_0},
     {"diamond_pickaxe", "Diamond Pickaxe", Block::Air, Release::V1_0},
     {"diamond_axe", "Diamond Axe", Block::Air, Release::V1_0},
     {"diamond_shovel", "Diamond Shovel", Block::Air, Release::V1_0},
     {"diamond_hoe", "Diamond Hoe", Block::Air, Release::V1_0},
     {"leather_helmet", "Leather Helmet", Block::Air, Release::V1_0},
     {"leather_chestplate", "Leather Chestplate", Block::Air, Release::V1_0},
     {"leather_leggings", "Leather Leggings", Block::Air, Release::V1_0},
     {"leather_boots", "Leather Boots", Block::Air, Release::V1_0},
     {"chainmail_helmet", "Chain Helmet", Block::Air, Release::V1_0},
     {"chainmail_chestplate", "Chain Chestplate", Block::Air, Release::V1_0},
     {"chainmail_leggings", "Chain Leggings", Block::Air, Release::V1_0},
     {"chainmail_boots", "Chain Boots", Block::Air, Release::V1_0},
     {"iron_helmet", "Iron Helmet", Block::Air, Release::V1_0},
     {"iron_chestplate", "Iron Chestplate", Block::Air, Release::V1_0},
     {"iron_leggings", "Iron Leggings", Block::Air, Release::V1_0},
     {"iron_boots", "Iron Boots", Block::Air, Release::V1_0},
     {"golden_helmet", "Gold Helmet", Block::Air, Release::V1_0},
     {"golden_chestplate", "Gold Chestplate", Block::Air, Release::V1_0},
     {"golden_leggings", "Gold Leggings", Block::Air, Release::V1_0},
     {"golden_boots", "Gold Boots", Block::Air, Release::V1_0},
     {"diamond_helmet", "Diamond Helmet", Block::Air, Release::V1_0},
     {"diamond_chestplate", "Diamond Chestplate", Block::Air, Release::V1_0},
     {"diamond_leggings", "Diamond Leggings", Block::Air, Release::V1_0},
     {"diamond_boots", "Diamond Boots", Block::Air, Release::V1_0}}};
struct MobDefinition {
    const char* id;
    Release since;
    unsigned health;
    float speed;
    Item drop;
    unsigned dropCount;
};
inline constexpr std::array<MobDefinition, 3> AnimalDefinitions{
    {{"minecraft:sheep", Release::V1_0, 8, 1.0f, Item::Wool, 2},
     {"minecraft:pig", Release::V1_0, 10, 1.3f, Item::Pork, 2},
     {"minecraft:cow", Release::V1_0, 12, 0.85f, Item::Leather, 2}}};
static_assert(
    [] {
        for (auto block : BlockTypes)
            if (!block.id)
                return false;
        for (auto item : ItemTypes)
            if (!item.id || !item.name)
                return false;
        return true;
    }(),
    "Every registered content ID requires traits");
enum class ToolKind { None, Sword, Pickaxe, Axe, Shovel, Hoe };
struct ToolStats {
    ToolKind kind = ToolKind::None;
    unsigned tier{}, durability{};
    float speed = 1, damage = 1, attackSpeed = 4;
};
inline ToolStats toolStats(Item item) {
    switch (item) {
    case Item::WoodSword:
        return {ToolKind::Sword, 1, 59, 2.0f, 4.0f, 1.6f};
    case Item::WoodPickaxe:
        return {ToolKind::Pickaxe, 1, 59, 2.0f, 2.0f, 1.2f};
    case Item::WoodAxe:
        return {ToolKind::Axe, 1, 59, 2.0f, 7.0f, 0.8f};
    case Item::WoodShovel:
        return {ToolKind::Shovel, 1, 59, 2.0f, 2.5f, 1.0f};
    case Item::WoodHoe:
        return {ToolKind::Hoe, 1, 59, 2.0f, 1.0f, 1.0f};
    case Item::StoneSword:
        return {ToolKind::Sword, 2, 131, 4.0f, 5.0f, 1.6f};
    case Item::StonePickaxe:
        return {ToolKind::Pickaxe, 2, 131, 4.0f, 3.0f, 1.2f};
    case Item::StoneAxe:
        return {ToolKind::Axe, 2, 131, 4.0f, 9.0f, 0.8f};
    case Item::StoneShovel:
        return {ToolKind::Shovel, 2, 131, 4.0f, 3.5f, 1.0f};
    case Item::StoneHoe:
        return {ToolKind::Hoe, 2, 131, 4.0f, 1.0f, 2.0f};
    case Item::Sword:
        return {ToolKind::Sword, 3, 250, 6.0f, 6.0f, 1.6f};
    case Item::Pickaxe:
        return {ToolKind::Pickaxe, 3, 250, 6.0f, 4.0f, 1.2f};
    case Item::IronAxe:
        return {ToolKind::Axe, 3, 250, 6.0f, 9.0f, 0.9f};
    case Item::IronShovel:
        return {ToolKind::Shovel, 3, 250, 6.0f, 4.5f, 1.0f};
    case Item::IronHoe:
        return {ToolKind::Hoe, 3, 250, 6.0f, 1.0f, 3.0f};
    case Item::GoldSword:
        return {ToolKind::Sword, 1, 32, 12.0f, 4.0f, 1.6f};
    case Item::GoldPickaxe:
        return {ToolKind::Pickaxe, 1, 32, 12.0f, 2.0f, 1.2f};
    case Item::GoldAxe:
        return {ToolKind::Axe, 1, 32, 12.0f, 7.0f, 1.0f};
    case Item::GoldShovel:
        return {ToolKind::Shovel, 1, 32, 12.0f, 2.5f, 1.0f};
    case Item::GoldHoe:
        return {ToolKind::Hoe, 1, 32, 12.0f, 1.0f, 1.0f};
    case Item::DiamondSword:
        return {ToolKind::Sword, 4, 1561, 8.0f, 7.0f, 1.6f};
    case Item::DiamondPickaxe:
        return {ToolKind::Pickaxe, 4, 1561, 8.0f, 5.0f, 1.2f};
    case Item::DiamondAxe:
        return {ToolKind::Axe, 4, 1561, 8.0f, 9.0f, 1.0f};
    case Item::DiamondShovel:
        return {ToolKind::Shovel, 4, 1561, 8.0f, 5.5f, 1.0f};
    case Item::DiamondHoe:
        return {ToolKind::Hoe, 4, 1561, 8.0f, 1.0f, 4.0f};
    default:
        return {};
    }
}
struct ArmorStats {
    int slot = -1;
    unsigned points{}, toughness{}, durability{};
};
inline ArmorStats armorStats(Item item) {
    switch (item) {
    case Item::LeatherHelmet:
        return {0, 1, 0, 55};
    case Item::LeatherChestplate:
        return {1, 3, 0, 80};
    case Item::LeatherLeggings:
        return {2, 2, 0, 75};
    case Item::LeatherBoots:
        return {3, 1, 0, 65};
    case Item::ChainHelmet:
        return {0, 2, 0, 165};
    case Item::ChainChestplate:
        return {1, 5, 0, 240};
    case Item::ChainLeggings:
        return {2, 4, 0, 225};
    case Item::ChainBoots:
        return {3, 1, 0, 195};
    case Item::IronHelmet:
        return {0, 2, 0, 165};
    case Item::IronChestplate:
        return {1, 6, 0, 240};
    case Item::IronLeggings:
        return {2, 5, 0, 225};
    case Item::IronBoots:
        return {3, 2, 0, 195};
    case Item::GoldHelmet:
        return {0, 2, 0, 77};
    case Item::GoldChestplate:
        return {1, 5, 0, 112};
    case Item::GoldLeggings:
        return {2, 3, 0, 105};
    case Item::GoldBoots:
        return {3, 1, 0, 91};
    case Item::DiamondHelmet:
        return {0, 3, 2, 363};
    case Item::DiamondChestplate:
        return {1, 8, 2, 528};
    case Item::DiamondLeggings:
        return {2, 6, 2, 495};
    case Item::DiamondBoots:
        return {3, 3, 2, 429};
    default:
        return {};
    }
}
inline unsigned itemDurability(Item item) {
    return toolStats(item).durability + armorStats(item).durability;
}
struct FoodStats {
    float hunger{}, saturation{};
};
inline FoodStats foodStats(Item item) {
    switch (item) {
    case Item::Pork:
        return {3.0f, 1.8f};
    case Item::Apple:
        return {4.0f, 2.4f};
    case Item::Bread:
        return {5.0f, 6.0f};
    case Item::CookedPork:
        return {8.0f, 12.8f};
    case Item::Beef:
        return {3.0f, 1.8f};
    case Item::Steak:
        return {8.0f, 12.8f};
    case Item::Chicken:
        return {2.0f, 1.2f};
    case Item::CookedChicken:
        return {6.0f, 7.2f};
    case Item::Mutton:
        return {2.0f, 1.2f};
    case Item::CookedMutton:
        return {6.0f, 9.6f};
    case Item::Carrot:
        return {3.0f, 3.6f};
    case Item::Potato:
        return {1.0f, 0.6f};
    case Item::BakedPotato:
        return {5.0f, 6.0f};
    case Item::Beetroot:
        return {1.0f, 1.2f};
    case Item::Cookie:
        return {2.0f, 0.4f};
    case Item::Melon:
        return {2.0f, 1.2f};
    case Item::PumpkinPie:
        return {8.0f, 4.8f};
    case Item::ChorusFruit:
        return {4.0f, 2.4f};
    default:
        return {};
    }
}
inline ToolKind preferredTool(Block block) {
    switch (block) {
    case Block::Air:
    case Block::Lava:
        return ToolKind::None;
    case Block::Grass:
    case Block::Dirt:
    case Block::Path:
        return ToolKind::Shovel;
    case Block::OakLog:
    case Block::BirchLog:
    case Block::SpruceLog:
    case Block::JungleLog:
    case Block::AcaciaLog:
    case Block::DarkOakLog:
    case Block::Planks:
    case Block::BirchPlanks:
    case Block::SprucePlanks:
    case Block::JunglePlanks:
    case Block::AcaciaPlanks:
    case Block::DarkOakPlanks:
    case Block::Chest:
    case Block::CraftingTable:
    case Block::Bookshelf:
        return ToolKind::Axe;
    case Block::OakLeaves:
    case Block::BirchLeaves:
    case Block::Glowstone:
    case Block::SeaLantern:
        return ToolKind::None;
    default:
        return block >= Block::WhiteWool && block <= Block::BlackWool ? ToolKind::None
                                                                      : ToolKind::Pickaxe;
    }
}
inline unsigned harvestTier(Block block) {
    switch (block) {
    case Block::Obsidian:
        return 4;
    case Block::GoldOre:
    case Block::DiamondOre:
    case Block::EmeraldOre:
    case Block::RedstoneOre:
    case Block::GoldBlock:
    case Block::DiamondBlock:
    case Block::EmeraldBlock:
    case Block::RedstoneBlock:
        return 3;
    case Block::IronOre:
    case Block::LapisOre:
    case Block::IronBlock:
    case Block::LapisBlock:
        return 2;
    default:
        return 1;
    }
}
inline bool canHarvest(Block block, Item tool) {
    auto stats = toolStats(tool);
    return preferredTool(block) != ToolKind::Pickaxe ||
           (stats.kind == ToolKind::Pickaxe && stats.tier >= harvestTier(block));
}
} // namespace voxel

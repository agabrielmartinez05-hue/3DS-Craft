#pragma once
#include <array>
namespace voxel {
enum class GuiSprite : unsigned {
    Hotbar,
    Selection,
    Offhand,
    HeartEmpty,
    HeartFull,
    HeartHalf,
    FoodEmpty,
    FoodFull,
    FoodHalf,
    Crosshair,
    Inventory,
    Chest,
    ArmorEmpty,
    ArmorFull,
    ArmorHalf,
    Furnace,
    FurnaceBurn,
    FurnaceCook,
    Count
};
inline constexpr std::array<const char*, unsigned(GuiSprite::Count)> GuiSprites{
    {"hud/hotbar", "hud/hotbar_selection", "hud/hotbar_offhand_left", "hud/heart/container",
     "hud/heart/full", "hud/heart/half", "hud/food_empty", "hud/food_full", "hud/food_half",
     "hud/crosshair", "gui/container/inventory", "gui/container/generic_54", "hud/armor_empty",
     "hud/armor_full", "hud/armor_half", "gui/container/furnace", "container/furnace/lit_progress",
     "container/furnace/burn_progress"}};
static_assert(
    [] {
        for (auto name : GuiSprites)
            if (!name)
                return false;
        return true;
    }(),
    "Every GUI sprite requires an asset path");
// One native GUI pixel per bottom-screen pixel. Nine visible hotbar slots.
constexpr float HotbarX = 69, HotbarY = 203;
} // namespace voxel

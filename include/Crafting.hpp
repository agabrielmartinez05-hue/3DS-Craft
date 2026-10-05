#pragma once
#include "Gameplay.hpp"
#include <string>
namespace voxel {
enum class RecipeKind { Shaped, Shapeless, Smelting, Brewing };
struct Ingredient {
    Item item = Item::Count;
    unsigned count = 1;
};
struct Recipe {
    std::string id;
    Release since = Release::V1_0;
    RecipeKind kind = RecipeKind::Shapeless;
    unsigned width{}, height{};
    std::array<Ingredient, 9> ingredients{};
    Item result = Item::Count;
    unsigned resultCount = 1;
};
using CraftGrid = std::array<Item, 9>; // Item::Count denotes an empty slot.
class RecipeBook {
  public:
    RecipeBook();
    const auto& recipes() const { return recipes_; }
    const Recipe* match(const CraftGrid& grid, unsigned width,
                        Release target = Release::V1_9) const;
    bool craft(const Recipe& recipe, Inventory& inventory) const;

  private:
    std::vector<Recipe> recipes_;
};
Item smeltingResult(Item input);
float fuelSeconds(Item fuel);
struct Furnace {
    Item input = Item::Count, fuel = Item::Count, output = Item::Count;
    unsigned inputCount{}, fuelCount{}, outputCount{};
    float burn{}, progress{};
    bool empty() const { return !inputCount && !fuelCount && !outputCount; }
    bool deposit(Inventory& inventory, Item item, bool asFuel, bool wholeStack);
    bool take(Inventory& inventory, unsigned slot);
    void tick(float seconds);
};
} // namespace voxel

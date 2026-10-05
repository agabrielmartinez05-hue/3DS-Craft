#pragma once
#include "Controls.hpp"
#include "Crafting.hpp"
#include "Renderer.hpp"
#include "World.hpp"
#include "WorldStore.hpp"

namespace voxel {
class App {
  public:
    explicit App(Renderer& renderer) : renderer_(renderer), saves_("sdmc:/3ds/3ds-craft/saves") {}
    void run();

  private:
    void nativeName();
    void travel(Dimension target);
    void respawn();
    void updateSurvival(float dt, Vec3 previous);
    void loadAssets();
    void menuInput(u32 down);
    void gameplay(u32 down, u32 held, float dt, double elapsed);
    void beginWorld(const std::string& id);
    void saveWorld(bool showProgress = true);
    Progress progress();
    void openEditor(bool seed);
    void drawMenu();
    Renderer& renderer_;
    WorldStore saves_;
    std::unique_ptr<GpuAssets> assets_;
    std::unique_ptr<World> world_;
    AnimalSystem animals_;
    Player player_;
    Inventory inventory_;
    DayNight cycle_;
    PerformanceMetrics metrics_;
    ControlSettings settings_;
    WorldSave session_;
    RecipeBook recipes_;
    GameMode creationMode_ = GameMode::Survival;
    WorldType creationType_ = WorldType::Normal;
    std::uint32_t nameRandom_ = 1;
    float hostileTime_{}, contactTime_{};
    bool deathProcessed_{}, debugMesh_{};
    GameState state_ = STATE_MAIN_MENU;
    std::vector<WorldSummary> worlds_;
    std::string worldId_, worldName_, name_ = "New World", seed_ = "42";
    std::uint32_t worldSeed_{};
    unsigned selected_{}, inventoryCursor_{}, inventoryPage_{}, key_{};
    bool chestOpen_{};
    BlockPos chestPosition_{}, furnacePosition_{};
    bool inventoryOpen_{}, editing_{}, editingSeed_{}, quit_{};
    TextEditor editor_;
    const char* message_ = "Circle Pad move | C-Stick look";
};
} // namespace voxel

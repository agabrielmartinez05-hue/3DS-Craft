#pragma once
#include "Animals.hpp"
#include "Crafting.hpp"
#include "GpuAssets.hpp"
#include "Menu.hpp"
#include "Metrics.hpp"
#include "Survival.hpp"
#include <citro2d.h>
#include <citro3d.h>

namespace voxel {
class World;
class Renderer {
  public:
    Renderer() = default;
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    void initialize();
    void draw(const World& world, const Camera& camera, const AnimalSystem& animals,
              const DayNight& cycle, const Inventory& inventory, const PerformanceMetrics& metrics,
              const char* message, const GpuAssets& assets, bool inventoryOpen,
              unsigned inventoryCursor, const Survival& survival, GameMode mode,
              Dimension dimension, bool debugMesh,
              const std::array<unsigned, ItemCount>* chest = nullptr, unsigned inventoryPage = 0);
    void menu(const std::string& title, const std::vector<MenuRow>& rows, unsigned selected,
              const std::string& subtitle = "",
              const char* footer = "D-pad move | L select | R back");
    void furnace(const Furnace& furnace, const GpuAssets& assets, Item selectedItem,
                 unsigned action, const char* message);
    void keyboard(const TextEditor& editor, unsigned selected, const char* title);
    void loading(const std::string& message, unsigned done, unsigned total);
    void promptError(const char* message);
    void errorScreen(const char* message) noexcept;

  private:
    void label(const char* message, float x, float y, float scale, u32 color);
    void shutdownGpu() noexcept;
    bool gfx_{}, c3d_{}, c2d_{}, programReady_{};
    C3D_RenderTarget* bottom_{};
    Vertex* animalVertices_{};
    std::vector<Vertex> animalStaging_;
    C2D_TextBuf text_{};
    DVLB_s* shader_{};
    shaderProgram_s program_{};
    int projection_{}, view_{}, sun_{}, lighting_{};
    C3D_AttrInfo attributes_{};
};
// Ends an active frame during exception unwinding as well as normal rendering.
class Frame {
  public:
    Frame();
    ~Frame();
    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;
};
} // namespace voxel

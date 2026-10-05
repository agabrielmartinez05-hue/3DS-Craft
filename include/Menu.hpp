#pragma once
#include <string>
#include <vector>

namespace voxel {
enum GameState {
    STATE_MAIN_MENU,
    STATE_WORLD_SELECT,
    STATE_CREATION,
    STATE_GAMEPLAY,
    STATE_ACTIONS,
    STATE_CRAFTING,
    STATE_FURNACE,
    STATE_SETTINGS
};
struct MenuRow {
    std::string label;
    bool enabled = true;
};
// Portable keyboard model; rendering and 3DS input remain in App/Renderer.
class TextEditor {
  public:
    void open(std::string initial, bool numeric);
    const std::string& text() const { return text_; }
    const std::vector<std::string>& keys() const { return keys_; }
    bool numeric() const { return numeric_; }
    // Returns true for OK; caller owns cancellation. No filesystem names derive from this input.
    bool press(unsigned key);

  private:
    std::string text_;
    std::vector<std::string> keys_;
    bool numeric_{};
};
} // namespace voxel

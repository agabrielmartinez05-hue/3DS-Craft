#include "Menu.hpp"
namespace voxel {
void TextEditor::open(std::string initial, bool numeric) {
    text_ = std::move(initial);
    numeric_ = numeric;
    keys_.clear();
    const std::string characters =
        numeric ? "1234567890" : "1234567890qwertyuiopasdfghjklzxcvbnm-_ ";
    for (char c : characters)
        keys_.push_back(c == ' ' ? "SP" : std::string(1, c));
    keys_.push_back("DEL");
    keys_.push_back("CLR");
    keys_.push_back("OK");
}
bool TextEditor::press(unsigned key) {
    if (key >= keys_.size())
        return false;
    const auto& value = keys_[key];
    if (value == "OK")
        return true;
    if (value == "DEL") {
        if (!text_.empty()) {
            // Native keyboard names may contain multi-byte UTF-8 characters.
            auto at = text_.size() - 1;
            while (at > 0 && (static_cast<unsigned char>(text_[at]) & 0xc0) == 0x80)
                --at;
            text_.erase(at);
        }
    } else if (value == "CLR")
        text_.clear();
    else if (text_.size() < (numeric_ ? 10u : 32u))
        text_ += value == "SP" ? " " : value;
    return false;
}
} // namespace voxel

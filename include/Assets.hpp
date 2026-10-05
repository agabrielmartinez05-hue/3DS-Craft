#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>
#include <vector>

namespace voxel {
struct Image {
    unsigned width{}, height{};
    std::vector<std::uint8_t> rgba;
};
Image loadPng(const char* path);
nlohmann::json loadJson(const char* path);
} // namespace voxel

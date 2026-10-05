#include "Assets.hpp"
#include "Error.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <png.h>

using namespace voxel;
int main(int argc, char** argv) {
    assert(argc == 2);
    const std::string dir = argv[1];
    std::filesystem::create_directories(dir);
    auto expectError = [](auto operation, const std::string& part) {
        bool failed = false;
        try {
            operation();
        } catch (const Error& e) {
            failed = true;
            assert(std::string(e.what()).find(part) != std::string::npos);
        }
        assert(failed);
    };
    const auto json = dir + "/world.json", pngPath = dir + "/pixel.png";
    {
        std::ofstream f(json);
        f << R"({"seed":1337,"blocks":["grass","stone"]})";
    }
    assert(loadJson(json.c_str())["seed"] == 1337);
    {
        std::ofstream f(json);
        f << "{broken}";
    }
    expectError([&] { loadJson(json.c_str()); }, "parse_error");
    {
        std::ofstream f(json);
        f << std::string(40, '[') + "0" + std::string(40, ']');
    }
    expectError([&] { loadJson(json.c_str()); }, "32 levels");
    {
        std::ofstream f(json);
        f << std::string(1024 * 1024 + 1, ' ');
    }
    expectError([&] { loadJson(json.c_str()); }, "1 MiB");
    expectError([&] { loadJson((dir + "/missing.json").c_str()); }, "unable to open");
    png_image png{};
    png.version = PNG_IMAGE_VERSION;
    png.width = 1;
    png.height = 1;
    png.format = PNG_FORMAT_RGBA;
    const unsigned char pixel[4] = {20, 180, 60, 255};
    assert(png_image_write_to_file(&png, pngPath.c_str(), 0, pixel, 0, nullptr));
    const auto loaded = loadPng(pngPath.c_str());
    assert(loaded.width == 1 && loaded.height == 1 &&
           loaded.rgba == std::vector<std::uint8_t>(pixel, pixel + 4));
    {
        std::ofstream f(pngPath);
        f << "invalid PNG";
    }
    expectError([&] { loadPng(pngPath.c_str()); }, "PNG '");
    expectError([&] { loadPng((dir + "/missing.png").c_str()); }, "PNG '");
    png.width = 1025;
    std::vector<unsigned char> wide(1025 * 4, 255);
    assert(png_image_write_to_file(&png, pngPath.c_str(), 0, wide.data(), 0, nullptr));
    expectError([&] { loadPng(pngPath.c_str()); }, "1024x1024");
    std::cout
        << "Asset tests passed: PNG pixels/limits/failures; JSON parsing/size/depth/failures\n";
}

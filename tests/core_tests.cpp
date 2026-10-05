#include "Core.hpp"
#include "Village.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>

using namespace voxel;
namespace {
int index(int x, int y, int z) {
    return (y + 1) * 324 + (z + 1) * 18 + x + 1;
}
Vec3 position(const Vertex& v) {
    return {v.x, v.y, v.z};
}
void verifyMesh(const Halo& halo) {
    std::vector<Vertex> vertices;
    meshSubChunk(halo, {}, vertices);
    assert(vertices.size() % 6 == 0);
    float area = 0;
    for (std::size_t i = 0; i < vertices.size(); i += 3) {
        auto a = position(vertices[i]), b = position(vertices[i + 1]),
             c = position(vertices[i + 2]);
        const auto normal = cross(b - a, c - a);
        const auto& vertex = vertices[i];
        assert(dot(normalized(normal), {vertex.nx, vertex.ny, vertex.nz}) > 0.999f);
        area += std::sqrt(dot(normal, normal)) * 0.5f;
        auto center = (a + b + c) * (1.0f / 3), n = normalized(normal);
        auto sample = [&](Vec3 p) {
            return halo[index(int(std::floor(p.x)), int(std::floor(p.y)), int(std::floor(p.z)))];
        };
        assert(sample(center - n * 0.01f) != Block::Air);
        assert(sample(center + n * 0.01f) == Block::Air);
    }
    int expected = 0;
    const int directions[6][3] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                                  {0, -1, 0}, {0, 0, 1},  {0, 0, -1}};
    for (int y = 0; y < 16; ++y)
        for (int z = 0; z < 16; ++z)
            for (int x = 0; x < 16; ++x) {
                if (halo[index(x, y, z)] == Block::Air)
                    continue;
                for (const auto& d : directions)
                    if (halo[index(x + d[0], y + d[1], z + d[2])] == Block::Air)
                        ++expected;
            }
    assert(std::abs(area - expected) < 0.01f);
}
} // namespace
void modernTerrain() {
    Terrain terrain(7, 2), same(7, 2), legacy(7, 1);
    Perlin noise(777);
    assert(std::abs(noise.noise(-0.0001f, 0.3f, 0.7f) - noise.noise(0.0001f, 0.3f, 0.7f)) < 0.001f);
    unsigned caves = 0, oak = 0, birch = 0, leaves = 0;
    for (int z = -32; z < 32; ++z)
        for (int x = -32; x < 32; ++x) {
            std::array<Block, WorldHeight> column;
            terrain.sampleColumn(x, z, column);
            const auto surface = terrain.surface(x, z);
            for (int y = 0; y < WorldHeight; ++y) {
                const auto b = column[y];
                if (y < 3)
                    assert(b != Block::Air);
                if (y >= surface.height - 4 && y < surface.height)
                    assert(b != Block::Air);
                if (b == Block::Air && y >= 3 && y < surface.height - 4)
                    ++caves;
                if (b == Block::OakLog)
                    ++oak;
                if (b == Block::BirchLog)
                    ++birch;
                if (b == Block::OakLeaves || b == Block::BirchLeaves)
                    ++leaves;
                if (x % 8 == 0 && z % 8 == 0) {
                    assert(b == terrain.block(x, y, z) && b == same.block(x, y, z));
                    assert(legacy.block(x, y, z) == Terrain::atHeight(surface, y));
                }
            }
        }
    std::cerr << "Terrain counts " << caves << " " << oak << " " << birch << " " << leaves << "\n";
    assert(caves > 100 && oak > 0 && birch > 0 && leaves > oak + birch);
    // Sample both sides of a negative/positive chunk boundary through actual meshes.
    for (int cx = -1; cx <= 0; ++cx) {
        Column col;
        generateColumn(col, terrain, cx, 0);
        bool underground = false;
        for (const auto& chunk : col.chunks) {
            if (chunk.key.y == 0)
                underground = true;
            for (std::size_t i = 0; i < chunk.vertices.size(); i += 3) {
                const auto a = position(chunk.vertices[i]), b = position(chunk.vertices[i + 1]),
                           c = position(chunk.vertices[i + 2]);
                const auto n = normalized(cross(b - a, c - a)), center = (a + b + c) * (1.0f / 3);
                const auto sample = [&](Vec3 p) {
                    return terrain.block(int(std::floor(p.x)), int(std::floor(p.y)),
                                         int(std::floor(p.z)));
                };
                assert(sample(center - n * 0.01f) != Block::Air &&
                       sample(center + n * 0.01f) == Block::Air);
            }
        }
        assert(underground);
    }
    std::cout << "Modern terrain: " << caves << " cave cells, " << oak << " oak logs, " << birch
              << " birch logs\n";
}
int main() {
    {
        Terrain current(42, 5), legacy(42, 4), nether(42, 5, Dimension::Nether);
        std::array<unsigned, BlockCount> counts{};
        for (int z = -48; z < 48; ++z)
            for (int x = -48; x < 48; ++x) {
                std::array<Block, WorldHeight> a{}, b{}, n{};
                current.sampleColumn(x, z, a);
                legacy.sampleColumn(x, z, b);
                nether.sampleColumn(x, z, n);
                for (int y = 0; y < WorldHeight; ++y) {
                    if (a[y] != b[y]) {
                        assert(b[y] == Block::Stone);
                        assert(a[y] == Block::CoalOre || a[y] == Block::IronOre ||
                               a[y] == Block::GoldOre || a[y] == Block::DiamondOre ||
                               a[y] == Block::RedstoneOre || a[y] == Block::LapisOre ||
                               a[y] == Block::EmeraldOre);
                    }
                    ++counts[unsigned(a[y])];
                    if (n[y] == Block::QuartzOre)
                        ++counts[unsigned(Block::QuartzOre)];
                }
                if (x % 16 == 0 && z % 16 == 0)
                    for (int y = 0; y < WorldHeight; ++y)
                        assert(current.block(x, y, z) == a[y]);
            }
        for (auto ore : {Block::CoalOre, Block::IronOre, Block::GoldOre, Block::DiamondOre,
                         Block::RedstoneOre, Block::LapisOre, Block::QuartzOre})
            assert(counts[unsigned(ore)] > 0);
    }

    {
        Terrain village(42, 4), legacy(42, 3);
        auto site = villageSite(village, 0, 0);
        assert(site.valid);
        unsigned chests = 0, paths = 0, logs = 0;
        for (int z = site.z - 15; z <= site.z + 15; ++z)
            for (int x = site.x - 15; x <= site.x + 15; ++x) {
                std::array<Block, WorldHeight> column;
                village.sampleColumn(x, z, column);
                for (int y = 0; y < WorldHeight; ++y) {
                    assert(column[y] == village.block(x, y, z));
                    chests += column[y] == Block::Chest;
                    paths += column[y] == Block::Path;
                    logs += column[y] == Block::OakLog;
                }
            }
        assert(chests == 4 && paths > 40 && logs >= 64 && !villageSite(legacy, 0, 0).valid);
    }
    modernTerrain();
    assert(floorDiv(-1, 16) == -1 && floorDiv(-16, 16) == -1 && floorDiv(-17, 16) == -2 &&
           floorDiv(16, 16) == 1);
    Perlin first(1337), same(1337), other(42);
    assert(first.noise(1, 2) == 0);
    assert(first.noise(-2.3f, 7.1f) == same.noise(-2.3f, 7.1f));
    assert(first.noise(-2.3f, 7.1f) != other.noise(-2.3f, 7.1f));
    assert(std::abs(first.noise(-0.0001f, 0.4f) - first.noise(0.0001f, 0.4f)) < 0.001f);
    Halo halo{};
    verifyMesh(halo);
    std::vector<Vertex> mesh;
    halo[index(0, 0, 0)] = Block::Grass;
    verifyMesh(halo);
    meshSubChunk(halo, {}, mesh);
    assert(mesh.size() == 36);
    halo[index(1, 0, 0)] = Block::Grass;
    verifyMesh(halo);
    meshSubChunk(halo, {}, mesh);
    assert(mesh.size() == 36);
    std::vector<MeshRange> ranges;
    meshSubChunk(halo, {}, mesh, &ranges);
    unsigned covered = 0;
    float largestU = 0;
    for (const auto& range : ranges) {
        assert(range.first == covered && range.count % 6 == 0 && range.material < 3);
        covered += range.count;
    }
    for (const auto& vertex : mesh)
        largestU = std::max(largestU, vertex.u);
    assert(covered == mesh.size() && largestU == 2); // Merged faces repeat across two blocks.
    halo.fill(Block::Stone);
    verifyMesh(halo);
    meshSubChunk(halo, {}, mesh);
    assert(mesh.empty());
    halo.fill(Block::Air);
    halo[index(15, 4, 4)] = Block::Stone;
    halo[index(16, 4, 4)] = Block::Stone;
    verifyMesh(halo);
    meshSubChunk(halo, {}, mesh);
    assert(mesh.size() == 30);
    std::uint32_t random = 19;
    for (auto& b : halo) {
        random = random * 1664525u + 1013904223u;
        b = static_cast<Block>((random >> 24) % 4);
    }
    verifyMesh(halo);

    Terrain terrain(1337, 1);
    Column column;
    generateColumn(column, terrain, -1, 0);
    std::size_t total = 0;
    for (const auto& chunk : column.chunks) {
        assert(!chunk.vertices.empty());
        total += chunk.vertices.size();
        for (std::size_t i = 0; i < chunk.vertices.size(); i += 3) {
            const auto a = position(chunk.vertices[i]), b = position(chunk.vertices[i + 1]),
                       c = position(chunk.vertices[i + 2]);
            const auto n = normalized(cross(b - a, c - a)), center = (a + b + c) * (1.0f / 3);
            auto sample = [&](Vec3 p) {
                return terrain.block(int(std::floor(p.x)), int(std::floor(p.y)),
                                     int(std::floor(p.z)));
            };
            assert(sample(center - n * 0.01f) != Block::Air &&
                   sample(center + n * 0.01f) == Block::Air);
        }
    }
    assert(total > 0);
    Camera camera;
    camera.eye = {0, 2, 0};
    assert(camera.visible({-1, 0, -10}, {1, 4, -8}));
    assert(!camera.visible({-1, 0, 8}, {1, 4, 10}));
    assert(!camera.visible({100, 0, -10}, {110, 4, -8}));
    assert(!camera.visible({-1, 0, -90}, {1, 4, -80}));
    assert(camera.visible({-1, 0, -1}, {1, 4, 1})); // Contains camera: conservative visibility.
    camera.yaw = Pi / 2;
    assert(camera.visible({8, 0, -1}, {10, 4, 1}));
    Player player;
    player.spawn(terrain);
    for (int i = 0; i < 240; ++i)
        player.step(terrain, {}, 1.0f / 60);
    assert(player.grounded);
    const float ground = player.feet.y;
    assert(ground >= terrain.surface(0, 0).height && ground < terrain.surface(0, 0).height + 0.1f);
    player.step(terrain, {0, 0, true}, 1.0f / 60);
    assert(player.feet.y > ground && !player.grounded);
    for (int i = 0; i < 240; ++i)
        player.step(terrain, {}, 1.0f / 60);
    assert(player.grounded && std::abs(player.feet.y - ground) < 0.05f);
    player.feet.y = 60;
    for (int i = 0; i < 240; ++i)
        player.step(terrain, {}, 0.05f);
    assert(player.grounded && player.feet.y >= terrain.surface(0, 0).height);
    // Find a real one-block rise and verify horizontal collision against its wall.
    bool wallChecked = false;
    for (int x = -80; x < 80 && !wallChecked; ++x) {
        const int h = terrain.surface(x, 0).height;
        if (terrain.surface(x + 1, 0).height <= h)
            continue;
        player.feet = {float(x) + 0.5f, float(h) + 0.02f, 0.5f};
        player.camera.yaw = 0;
        player.verticalSpeed = 0;
        for (int i = 0; i < 60; ++i)
            player.step(terrain, {0, 1, false}, 1.0f / 60);
        assert(player.feet.x <= float(x + 1) - 0.29f);
        wallChecked = true;
    }
    assert(wallChecked);
    player.feet.x = WorldLimit - 0.01f;
    for (int i = 0; i < 60; ++i)
        player.step(terrain, {0, 1, false}, 0.05f);
    assert(player.feet.x <= WorldLimit);
    std::cout << "Core tests passed: noise, greedy mesh area/winding/halos, negative chunks, "
                 "frustum, collision/jump/terminal fall\n";
}

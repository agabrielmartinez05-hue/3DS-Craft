#include "Error.hpp"
#include "World.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

using namespace voxel;
namespace {
void settle(World& world, Vec3 position) {
    for (int i = 0; i < 60; ++i)
        world.update(position);
}
void synchronous() {
    World world(1337, 1);
    const Vec3 position{0.5f, 30, 0.5f};
    settle(world, position);
    const auto empty = ChunkKey{0, 3, 0};
    assert(world.loaded(0, 0) && world.chunks().size() < 100 && !world.chunks().empty());
    assert(!world.chunks().count(empty));
    assert(world.chunks().size() == testLiveAllocations);
    const auto baseline = world.meshBytes();
    const auto allocationCount = testTotalAllocations;
    settle(world, position);
    assert(allocationCount == testTotalAllocations); // No zero-mesh retry allocations.
    world.blocks.set({8, 55, 8}, Block::Grass);
    world.update(position);
    assert(world.chunks().count(empty) && world.chunks().at(empty)->count == 36);
    world.blocks.set({8, 55, 8}, Block::Air);
    world.update(position);
    assert(!world.chunks().count(empty) && world.meshBytes() == baseline);
    const auto allocationsAfterErase = testTotalAllocations;
    settle(world, position);
    assert(testTotalAllocations == allocationsAfterErase);

    world.blocks.set({15, 55, 8}, Block::Stone);
    world.blocks.set({16, 55, 8}, Block::Stone);
    world.update(position);
    assert(world.chunks().at({0, 3, 0})->count == 30 && world.chunks().at({1, 3, 0})->count == 30);
    world.blocks.set({15, 55, 8}, Block::Air);
    world.update(position);
    assert(!world.chunks().count({0, 3, 0}) && world.chunks().at({1, 3, 0})->count == 36);
    world.blocks.set({16, 55, 8}, Block::Air);
    world.update(position);
    world.blocks.set({8, 47, 8}, Block::Stone);
    world.blocks.set({8, 48, 8}, Block::Stone);
    world.update(position);
    assert(world.chunks().at({0, 2, 0})->count == 30 && world.chunks().at({0, 3, 0})->count == 30);
    world.blocks.set({8, 47, 8}, Block::Air);
    world.blocks.set({8, 48, 8}, Block::Air);
    world.update(position);
    assert(!world.chunks().count({0, 2, 0}) && !world.chunks().count({0, 3, 0}));

    assert(!world.chunks().count({0, 0, 0}));
    world.blocks.set({8, 15, 8}, Block::Air);
    world.update(position);
    assert(world.chunks().count({0, 0, 0})); // Enclosed solids wake when digging exposes them.
    world.blocks.set({8, 55, 8}, Block::Dirt);
    world.update(position);
    settle(world, {256, 30, 256});
    assert(!world.chunks().count(empty));
    settle(world, position);
    assert(world.blocks.block(8, 55, 8) == Block::Dirt && world.chunks().count(empty));
    const auto bytes = world.meshBytes();
    auto old = world.chunks().at(empty)->vertices;
    world.blocks.set({9, 55, 8}, Block::Stone);
    testFailAllocation = true;
    bool failed = false;
    try {
        world.update(position);
    } catch (const Error&) {
        failed = true;
    }
    testFailAllocation = false;
    assert(failed && world.meshBytes() == bytes && world.chunks().at(empty)->vertices == old);
}
void modernAtlasWorld() {
    BlockModels models;
    for (unsigned b = 1; b < BlockCount; ++b) {
        ModelElement e;
        for (auto& face : e.faces) {
            face.present = true;
            face.cull = true;
            face.uv = {0.1f, 0.2f, 0.15f, 0.25f};
        }
        models[b].elements.push_back(e);
        models[b].occludes = BlockTypes[b].layer == RenderLayer::Opaque;
    }
    for (auto dim : {Dimension::Nether, Dimension::End}) {
        World dimension(7, 3, &models, dim);
        dimension.prime({0.5f, 30, 0.5f});
        settle(dimension, {0.5f, 30, 0.5f});
        assert(dimension.vertexCount() > 0 && dimension.meshBytes() < 12 * 1024 * 1024);
        std::cout << "Dimension " << unsigned(dim) << " VBO bytes: " << dimension.meshBytes()
                  << "\n";
    }
    World world(7, 2, &models);
    const auto beforeFlush = testCacheFlushes;
    world.prime({0.5f, 30, 0.5f});
    assert(world.vertexCount() > 0 && world.loaded(0, 0));
    assert(testCacheFlushes > beforeFlush);
    for (const auto& entry : world.chunks()) {
        unsigned end = 0;
        for (const auto& range : entry.second->ranges) {
            assert(range.material <= 1 && range.first == end && range.count > 0);
            end += range.count;
        }
        assert(end == unsigned(entry.second->count));
    }
    settle(world, {0.5f, 30, 0.5f});
    assert(world.loaded(0, 0) && world.meshBytes() < 12 * 1024 * 1024);
    const auto allocations = testTotalAllocations;
    settle(world, {0.5f, 30, 0.5f});
    assert(testTotalAllocations == allocations);
    for (int z = -8; z < 8; ++z)
        for (int x = -8; x < 8; ++x)
            for (int y = 0; y < 48; ++y)
                assert(world.blocks.block(x, y, z) == world.blocks.terrain().block(x, y, z));
    world.blocks.set({8, 55, 8}, Block::BirchLog);
    world.update({0.5f, 30, 0.5f});
    assert(world.chunks().at({0, 3, 0})->count == 36);
    world.blocks.set({8, 55, 8}, Block::Air);
    world.update({0.5f, 30, 0.5f});
    assert(!world.chunks().count({0, 3, 0}));
    std::cout << "Atlas world VBO bytes (25 columns with caves/trees): " << world.meshBytes()
              << "\n";
}
void asynchronous() {
    testWorkerThreads = true;
    World world(1337, 1);
    assert(world.threaded());
    const Vec3 position{0.5f, 30, 0.5f};
    world.update(position);                     // Launch snapshot at revision zero.
    world.blocks.set({8, 55, 8}, Block::Grass); // Make pending result stale before adoption.
    for (int i = 0; i < 1000 && !world.loaded(0, 0); ++i) {
        world.update(position);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(world.loaded(0, 0) && world.chunks().count({0, 3, 0}));
    assert(world.blocks.block(8, 55, 8) == Block::Grass);
}
} // namespace
int main() {
    synchronous();
    assert(testLiveAllocations == 0);
    modernAtlasWorld();
    assert(testLiveAllocations == 0);
    {
        World end(7, 3, nullptr, Dimension::End);
        end.prime({200, -20, 200});
        assert(end.vertexCount() == 0);
    }
    asynchronous();
    assert(testLiveAllocations == 0);
    std::cout
        << "World tests passed: sparse eviction/reactivation/no idle rebuilds, boundary edits, "
           "unload/reload, atomic VBO replacement failure, stale worker rejection, cleanup\n";
}

#include "World.hpp"
#include "Error.hpp"
#include <algorithm>
#include <citro3d.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>

namespace voxel {
namespace {
struct Lock {
    LightLock* value;
    explicit Lock(LightLock& lock) : value(&lock) { LightLock_Lock(value); }
    ~Lock() { LightLock_Unlock(value); }
};
constexpr std::size_t MeshBudget = 12 * 1024 * 1024;
} // namespace
TerrainWorker::TerrainWorker(const Terrain& terrain, const BlockModels* models)
    : terrain_(terrain), models_(models) {
    LightLock_Init(&lock_);
    LightEvent_Init(&wake_, RESET_ONESHOT);
    // Core 2 is an extra application core on New 3DS, subject to loader permissions.
    // Failure falls back to synchronous generation; no system-core takeover.
    thread_ = threadCreate(entry, this, 128 * 1024, 0x31, 2, false);
}
TerrainWorker::~TerrainWorker() {
    {
        Lock guard(lock_);
        stop_ = true;
    }
    if (thread_) {
        LightEvent_Signal(&wake_);
        threadJoin(thread_, U64_MAX);
        threadFree(thread_);
    }
}
bool TerrainWorker::idle() {
    Lock guard(lock_);
    return !busy_ && !result_ && !failure_[0];
}
void TerrainWorker::request(int x, int z, std::uint64_t revision, std::vector<BlockEdit> edits) {
    {
        Lock guard(lock_);
        if (busy_ || result_ || failure_[0])
            return;
        x_ = x;
        z_ = z;
        revision_ = revision;
        edits_ = std::move(edits);
        busy_ = true;
    }
    if (thread_)
        LightEvent_Signal(&wake_);
    else {
        auto col = std::make_unique<Column>();
        generateColumn(*col, terrain_, x, z, edits_, models_);
        col->revision = revision_;
        std::vector<BlockEdit>().swap(edits_);
        Lock guard(lock_);
        result_ = std::move(col);
        busy_ = false;
    }
}
void TerrainWorker::entry(void* self) {
    static_cast<TerrainWorker*>(self)->run();
}
void TerrainWorker::run() {
    for (;;) {
        LightEvent_Wait(&wake_);
        int x, z;
        std::uint64_t revision;
        std::vector<BlockEdit> edits;
        {
            Lock guard(lock_);
            if (stop_)
                return;
            x = x_;
            z = z_;
            revision = revision_;
            edits.swap(edits_);
        }
        try {
            auto col = std::make_unique<Column>();
            generateColumn(*col, terrain_, x, z, edits, models_);
            col->revision = revision;
            Lock guard(lock_);
            result_ = std::move(col);
            busy_ = false;
        } catch (const std::exception& e) {
            Lock guard(lock_);
            std::snprintf(failure_, sizeof(failure_), "Terrain worker: %s", e.what());
            busy_ = false;
        } catch (...) {
            Lock guard(lock_);
            std::snprintf(failure_, sizeof(failure_), "Terrain worker: unknown exception");
            busy_ = false;
        }
    }
}
std::unique_ptr<Column> TerrainWorker::take() {
    Lock guard(lock_);
    if (failure_[0])
        throw Error("%s", failure_);
    return std::move(result_);
}
GpuMesh::~GpuMesh() {
    if (vertices)
        linearFree(vertices);
}
World::~World() {
    // Drain the prior frame before member destructors free VBOs still referenced by GPU.
    if (C3D_FrameBegin(0))
        C3D_FrameEnd(0);
}
bool World::loaded(int x, int z) const {
    return std::any_of(loadedColumns_.begin(), loadedColumns_.end(),
                       [&](BlockPos p) { return p.x == x && p.z == z; });
}
void World::replace(SubChunk chunk) {
    auto old = chunks_.find(chunk.key);
    const std::size_t oldBytes = old == chunks_.end() ? 0 : old->second->count * sizeof(Vertex);
    const std::size_t bytes = chunk.vertices.size() * sizeof(Vertex);
    if (!bytes) {
        if (old != chunks_.end()) {
            chunks_.erase(old);
            meshBytes_ -= oldBytes;
        }
        return;
    }
    if (meshBytes_ - oldBytes + bytes > MeshBudget)
        throw Error("World VBO budget exceeded (12 MiB)");
    auto mesh = std::make_unique<GpuMesh>();
    mesh->vertices = static_cast<Vertex*>(linearAlloc(bytes));
    if (!mesh->vertices)
        throw Error("Out of linear RAM: chunk (%d,%d,%d), %lu bytes", chunk.key.x, chunk.key.y,
                    chunk.key.z, static_cast<unsigned long>(bytes));
    std::memcpy(mesh->vertices, chunk.vertices.data(), bytes);
    GSPGPU_FlushDataCache(mesh->vertices, bytes);
    mesh->count = static_cast<int>(chunk.vertices.size());
    mesh->ranges = std::move(chunk.ranges);
    chunks_[chunk.key] = std::move(mesh);
    meshBytes_ = meshBytes_ - oldBytes + bytes;
    // CPU mesh destroyed here. FrameEnd(0) flushes the VBO and Citro2D buffers.
}
void World::releaseMeshes() {
    chunks_.clear();
    loadedColumns_.clear();
    meshBytes_ = 0;
}
unsigned World::vertexCount() const {
    unsigned n = 0;
    for (const auto& c : chunks_)
        n += unsigned(c.second->count);
    return n;
}
void World::prime(Vec3 player) {
    const auto key = chunkOf({int(std::floor(player.x)), 0, int(std::floor(player.z))});
    for (int dz = -1; dz <= 1; ++dz)
        for (int dx = -1; dx <= 1; ++dx) {
            const int x = key.x + dx, z = key.z + dz;
            if (loaded(x, z))
                continue;
            Column col;
            generateColumn(col, blocks.terrain(), x, z, blocks.snapshot(x, z), models_);
            for (auto& chunk : col.chunks)
                replace(std::move(chunk));
            loadedColumns_.push_back({x, 0, z});
        }
    if (chunks_.empty() && blocks.terrain().dimension() != Dimension::End)
        throw Error("Terrain bootstrap generated no visible geometry");
}
void World::update(Vec3 player) {
    const int cx = floorDiv(int(std::floor(player.x)), 16),
              cz = floorDiv(int(std::floor(player.z)), 16);
    const auto inRange = [&](int x, int z) {
        return std::abs(x - cx) <= RenderRadius && std::abs(z - cz) <= RenderRadius;
    };
    for (auto it = chunks_.begin(); it != chunks_.end();) {
        if (!inRange(it->first.x, it->first.z)) {
            meshBytes_ -= it->second->count * sizeof(Vertex);
            it = chunks_.erase(it);
        } else
            ++it;
    }
    loadedColumns_.erase(std::remove_if(loadedColumns_.begin(), loadedColumns_.end(),
                                        [&](BlockPos p) { return !inRange(p.x, p.z); }),
                         loadedColumns_.end());
    // Edit-driven work only; no polling of absent sub-chunks.
    for (const auto key : blocks.takeDirty())
        if (loaded(key.x, key.z))
            replace(
                generateSubChunk(blocks.terrain(), key, blocks.snapshot(key.x, key.z), models_));
    if (auto ready = worker_.take();
        ready && inRange(ready->x, ready->z) && ready->revision == blocks.revision()) {
        for (auto& chunk : ready->chunks)
            replace(std::move(chunk));
        loadedColumns_.push_back({ready->x, 0, ready->z});
    }
    // Results generated before an edit are discarded, never allowed to overwrite it.
    if (!worker_.idle())
        return;
    int best = std::numeric_limits<int>::max(), bx = 0, bz = 0;
    for (int dz = -RenderRadius; dz <= RenderRadius; ++dz)
        for (int dx = -RenderRadius; dx <= RenderRadius; ++dx) {
            if (!loaded(cx + dx, cz + dz) && dx * dx + dz * dz < best) {
                best = dx * dx + dz * dz;
                bx = cx + dx;
                bz = cz + dz;
            }
        }
    if (best != std::numeric_limits<int>::max())
        worker_.request(bx, bz, blocks.revision(), blocks.snapshot(bx, bz));
}
} // namespace voxel

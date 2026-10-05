#include "Core.hpp"
#include "Village.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace voxel {
float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
Vec3 normalized(Vec3 v) {
    const float n = std::sqrt(dot(v, v));
    return n > 0 ? v * (1 / n) : Vec3{};
}
int floorDiv(int n, int d) {
    return n / d - (n % d < 0 ? 1 : 0);
}
namespace {
float fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}
float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}
float gradient(unsigned hash, float x, float y) {
    switch (hash & 7) {
    case 0:
        return x + y;
    case 1:
        return -x + y;
    case 2:
        return x - y;
    case 3:
        return -x - y;
    case 4:
        return x;
    case 5:
        return -x;
    case 6:
        return y;
    default:
        return -y;
    }
}
int haloIndex(int x, int y, int z) {
    return (y + 1) * 18 * 18 + (z + 1) * 18 + x + 1;
}
} // namespace
Perlin::Perlin(std::uint32_t seed) {
    std::iota(permutation_.begin(), permutation_.begin() + 256, 0);
    // Explicit integer PRNG: the same seed produces the same world on host and ARM.
    for (unsigned i = 255; i > 0; --i) {
        seed = seed * 1664525u + 1013904223u;
        std::swap(permutation_[i], permutation_[seed % (i + 1)]);
    }
    std::copy_n(permutation_.begin(), 256, permutation_.begin() + 256);
}
float Perlin::noise(float x, float y) const {
    const int ix = static_cast<int>(std::floor(x)), iy = static_cast<int>(std::floor(y));
    x -= ix;
    y -= iy;
    const int a = permutation_[ix & 255] + (iy & 255);
    const int b = permutation_[(ix + 1) & 255] + (iy & 255);
    return lerp(lerp(gradient(permutation_[a], x, y), gradient(permutation_[b], x - 1, y), fade(x)),
                lerp(gradient(permutation_[a + 1], x, y - 1),
                     gradient(permutation_[b + 1], x - 1, y - 1), fade(x)),
                fade(y));
}
float Perlin::noise(float x, float y, float z) const {
    const int ix = int(std::floor(x)), iy = int(std::floor(y)), iz = int(std::floor(z));
    x -= ix;
    y -= iy;
    z -= iz;
    const auto grad = [](unsigned h, float a, float b, float c) {
        h &= 15;
        const float u = h < 8 ? a : b;
        const float v = h < 4 ? b : (h == 12 || h == 14 ? a : c);
        return (h & 1 ? -u : u) + (h & 2 ? -v : v);
    };
    const auto corner = [&](int dx, int dy, int dz) {
        const auto h =
            permutation_[permutation_[permutation_[(ix + dx) & 255] + ((iy + dy) & 255)] +
                         ((iz + dz) & 255)];
        return grad(h, x - dx, y - dy, z - dz);
    };
    return lerp(lerp(lerp(corner(0, 0, 0), corner(1, 0, 0), fade(x)),
                     lerp(corner(0, 1, 0), corner(1, 1, 0), fade(x)), fade(y)),
                lerp(lerp(corner(0, 0, 1), corner(1, 0, 1), fade(x)),
                     lerp(corner(0, 1, 1), corner(1, 1, 1), fade(x)), fade(y)),
                fade(z));
}
float Perlin::fractal(float x, float y, int octaves) const {
    float result = 0, amplitude = 1, weight = 0;
    for (int i = 0; i < octaves; ++i) {
        result += noise(x, y) * amplitude;
        weight += amplitude;
        x *= 2;
        y *= 2;
        amplitude *= 0.5f;
    }
    return weight > 0 ? result / weight : 0;
}
Surface Terrain::surface(int x, int z) const {
    const float climate = climate_.fractal((x + 371) * 0.003f, (z - 219) * 0.003f, 2);
    const Biome biome = climate < -0.16f  ? Biome::Rocky
                        : climate > 0.12f ? Biome::Hills
                                          : Biome::Plains;
    const float amplitude = biome == Biome::Plains ? 9.0f : 22.0f;
    const int h = static_cast<int>(28 + amplitude * height_.fractal(x * 0.012f, z * 0.012f));
    return {std::clamp(h, 8, WorldHeight - 5), biome};
}
Block Terrain::atHeight(Surface s, int y) {
    if (y < 0)
        return Block::Stone; // Solid bedrock below world, no underside faces.
    if (y >= s.height)
        return Block::Air;
    if (s.biome == Biome::Rocky)
        return Block::Stone;
    if (y == s.height - 1)
        return Block::Grass;
    return y >= s.height - 4 ? Block::Dirt : Block::Stone;
}
Block Terrain::vegetation(int x, int y, int z) const {
    Block leaves = Block::Air;
    for (int cz = floorDiv(z - 2, 8); cz <= floorDiv(z + 2, 8); ++cz)
        for (int cx = floorDiv(x - 2, 8); cx <= floorDiv(x + 2, 8); ++cx) {
            std::uint32_t hash =
                seed_ ^ (std::uint32_t(cx) * 0x9e3779b9u) ^ (std::uint32_t(cz) * 0x85ebca6bu);
            hash ^= hash >> 16;
            hash *= 0x7feb352du;
            hash ^= hash >> 15;
            if ((hash & 3) != 0)
                continue;
            const int tx = cx * 8 + 1 + int((hash >> 3) % 6),
                      tz = cz * 8 + 1 + int((hash >> 8) % 6);
            const int dx = std::abs(x - tx), dz = std::abs(z - tz);
            if (dx > 2 || dz > 2)
                continue;
            const auto ground = surface(tx, tz);
            if (atHeight(ground, ground.height - 1) != Block::Grass)
                continue;
            const int h = 4 + int((hash >> 13) % 3), dy = y - ground.height;
            const bool birch = (hash & 0x10000u) != 0;
            if (!dx && !dz && dy >= 0 && dy < h)
                return birch ? Block::BirchLog : Block::OakLog;
            if (dy >= h - 2 && dy <= h + 1) {
                const int radius = dy == h + 1 ? 1 : 2;
                // Round, layered crowns; deterministic corner gaps give an organic outline.
                if (dx <= radius && dz <= radius && (dx + dz < radius * 2 || ((hash >> 18) & 1)))
                    leaves = birch ? Block::BirchLeaves : Block::OakLeaves;
            }
        }
    return leaves;
}
Block Terrain::sample(int x, int y, int z, Surface ground) const {
    if (dimension_ == Dimension::Nether) {
        const int floor = 8 + int(height_.noise(x * 0.025f, z * 0.025f) * 4);
        if (y < floor)
            return (x * x + z * z < 9) ? Block::Obsidian : Block::Netherrack;
        if (y == floor && ((x / 7 + z / 7) % 5 == 0))
            return Block::Lava;
        if (y > 52 || (y > 18 && caves_.noise(x * 0.065f, y * 0.065f, z * 0.065f) > 0.48f))
            return Block::Netherrack;
        return Block::Air;
    }
    if (dimension_ == Dimension::End) {
        const float radius = std::sqrt(float(x) * x + float(z) * z);
        const float edge = 75 + height_.noise(x * 0.025f, z * 0.025f) * 14;
        const int top = 30 + int(height_.noise(x * 0.035f, z * 0.035f) * 4);
        const int bottom = 12 + int(radius * 0.23f);
        return radius < edge && y >= bottom && y < top ? Block::EndStone : Block::Air;
    }
    if (type_ == WorldType::Superflat)
        return y < 1 ? Block::Stone : y < 3 ? Block::Dirt : y == 3 ? Block::Grass : Block::Air;
    Block value = atHeight(ground, y);
    if (version_ == 1)
        return value;
    if (value != Block::Air && y >= 3 && y < ground.height - 4) {
        const float a = caves_.noise(x * 0.052f, y * 0.075f, z * 0.052f);
        const float b = caves_.noise(x * 0.047f + 47.3f, y * 0.068f + 17.1f, z * 0.047f - 23.8f);
        if (std::abs(a) < 0.085f && std::abs(b) < 0.10f)
            return Block::Air;
    }
    if (version_ >= 3 && value == Block::Stone && y > 3 && y < 24 &&
        caves_.noise(x * 0.23f + 31, y * 0.23f, z * 0.23f) > 0.58f)
        return Block::IronOre;
    if (value == Block::Air && y >= ground.height && y <= ground.height + 10)
        return vegetation(x, y, z);
    return value;
}
void Terrain::sampleColumn(int x, int z, std::array<Block, WorldHeight>& out) const {
    const auto ground = surface(x, z);
    for (int y = 0; y < WorldHeight; ++y)
        out[y] = sample(x, y, z, ground);
    if (version_ >= 5 && type_ == WorldType::Normal)
        for (int y = 2; y < WorldHeight - 2; ++y) {
            if (out[y] != Block::Stone && out[y] != Block::Netherrack)
                continue;
            const int cx = floorDiv(x, 4), cy = y / 4, cz = floorDiv(z, 4);
            unsigned h = seed_ ^ (unsigned(cx) * 0x9e3779b9u) ^ (unsigned(cy) * 0x85ebca6bu) ^
                         (unsigned(cz) * 0xc2b2ae35u);
            h ^= h >> 16;
            h *= 0x7feb352du;
            h ^= h >> 15;
            int dx = x - cx * 4 - int((h >> 8) & 3), dy = y % 4 - int((h >> 10) & 3),
                dz = z - cz * 4 - int((h >> 12) & 3);
            if (dx * dx + dy * dy + dz * dz > 5)
                continue;
            const unsigned roll = h % 100;
            if (out[y] == Block::Netherrack) {
                if (roll < 15)
                    out[y] = Block::QuartzOre;
                continue;
            }
            if (dimension_ != Dimension::Overworld)
                continue;
            if (roll < 15)
                out[y] = Block::CoalOre;
            else if (roll < 25 && y < 40)
                out[y] = Block::IronOre;
            else if (roll < 30 && y < 24)
                out[y] = Block::GoldOre;
            else if (roll < 34 && y < 16)
                out[y] = Block::RedstoneOre;
            else if (roll < 37 && y < 24)
                out[y] = Block::LapisOre;
            else if (roll < 40 && y < 12)
                out[y] = Block::DiamondOre;
            else if (roll == 40 && y < 20 && ground.biome == Biome::Hills)
                out[y] = Block::EmeraldOre;
        }
    if (version_ >= 4)
        stampVillageColumn(*this, x, z, out);
}
Block Terrain::block(int x, int y, int z) const {
    if (y >= WorldHeight)
        return Block::Air;
    if (y < 0)
        return dimension_ == Dimension::End ? Block::Air : Block::Stone;
    if (version_ >= 4) {
        std::array<Block, WorldHeight> column;
        sampleColumn(x, z, column);
        return column[y];
    }
    return sample(x, y, z, surface(x, z));
}
ChunkKey chunkOf(BlockPos p) {
    return {floorDiv(p.x, 16), floorDiv(p.y, 16), floorDiv(p.z, 16)};
}
void meshSubChunk(const Halo& halo, Vec3 origin, std::vector<Vertex>& out,
                  std::vector<MeshRange>* ranges) {
    out.clear();
    std::array<int, 256> mask{};
    std::array<std::vector<Vertex>, (BlockCount - 1) * 3> buckets;
    if (ranges)
        ranges->clear();
    for (int axis = 0; axis < 3; ++axis) {
        const int u = (axis + 1) % 3, v = (axis + 2) % 3;
        for (int slice = -1; slice < 16; ++slice) {
            for (int j = 0; j < 16; ++j)
                for (int i = 0; i < 16; ++i) {
                    int p[3]{};
                    p[axis] = slice;
                    p[u] = i;
                    p[v] = j;
                    const auto a = halo[haloIndex(p[0], p[1], p[2])];
                    ++p[axis];
                    const auto b = halo[haloIndex(p[0], p[1], p[2])];
                    int face = 0;
                    if (a != Block::Air && b == Block::Air && slice >= 0)
                        face = static_cast<int>(a);
                    if (a == Block::Air && b != Block::Air && slice < 15)
                        face = -static_cast<int>(b);
                    mask[j * 16 + i] = face;
                }
            for (int j = 0; j < 16; ++j)
                for (int i = 0; i < 16;) {
                    const int face = mask[j * 16 + i];
                    if (!face) {
                        ++i;
                        continue;
                    }
                    int width = 1, height = 1;
                    while (i + width < 16 && mask[j * 16 + i + width] == face)
                        ++width;
                    bool extend = true;
                    while (j + height < 16 && extend) {
                        for (int k = 0; k < width; ++k)
                            if (mask[(j + height) * 16 + i + k] != face) {
                                extend = false;
                                break;
                            }
                        if (extend)
                            ++height;
                    }
                    float p[3]{};
                    p[axis] = static_cast<float>(slice + 1);
                    p[u] = static_cast<float>(i);
                    p[v] = static_cast<float>(j);
                    float du[3]{}, dv[3]{};
                    du[u] = static_cast<float>(width);
                    dv[v] = static_cast<float>(height);
                    const Vec3 a = {p[0] + origin.x, p[1] + origin.y, p[2] + origin.z};
                    const Vec3 b = a + Vec3{du[0], du[1], du[2]}, d = a + Vec3{dv[0], dv[1], dv[2]},
                               c = b + (d - a);
                    const Vec3 corners[4] = {a, b, c, d};
                    const int positive[6] = {0, 1, 2, 0, 2, 3}, negative[6] = {0, 2, 1, 0, 3, 2};
                    const auto indices = face > 0 ? positive : negative;
                    float normal[3]{};
                    normal[axis] = face > 0 ? 1.0f : -1.0f;
                    const unsigned material =
                        (std::abs(face) - 1) * 3 + (axis == 1 ? (face > 0 ? 0 : 1) : 2);
                    for (int k = 0; k < 6; ++k) {
                        const auto q = corners[indices[k]];
                        const float texU = axis == 0 ? q.z - origin.z : q.x - origin.x;
                        const float texV = axis == 1 ? q.z - origin.z : q.y - origin.y;
                        buckets[material].push_back(
                            {q.x, q.y, q.z, 1, 1, 1, normal[0], normal[1], normal[2], texU, texV});
                    }
                    for (int h = 0; h < height; ++h)
                        for (int w = 0; w < width; ++w)
                            mask[(j + h) * 16 + i + w] = 0;
                    i += width;
                }
        }
    }
    for (unsigned material = 0; material < buckets.size(); ++material) {
        const auto& bucket = buckets[material];
        if (bucket.empty())
            continue;
        if (ranges)
            ranges->push_back({material, unsigned(out.size()), unsigned(bucket.size())});
        out.insert(out.end(), bucket.begin(), bucket.end());
    }
}
namespace {
using ColumnGrid = std::array<std::array<Block, WorldHeight>, 18 * 18>;
ColumnGrid sampleColumns(const Terrain& terrain, int cx, int cz) {
    ColumnGrid grid{};
    for (int z = -1; z <= 16; ++z)
        for (int x = -1; x <= 16; ++x)
            terrain.sampleColumn(cx * 16 + x, cz * 16 + z, grid[(z + 1) * 18 + x + 1]);
    return grid;
}
SubChunk buildMesh(const ColumnGrid& columns, ChunkKey key, const std::vector<BlockEdit>& edits,
                   const BlockModels* models, bool solidBottom) {
    Halo halo{};
    for (int y = -1; y <= 16; ++y)
        for (int z = -1; z <= 16; ++z)
            for (int x = -1; x <= 16; ++x) {
                const int wy = key.y * 16 + y;
                halo[haloIndex(x, y, z)] = wy < 0 ? (solidBottom ? Block::Stone : Block::Air)
                                           : wy >= WorldHeight ? Block::Air
                                                               : columns[(z + 1) * 18 + x + 1][wy];
            }
    for (const auto& edit : edits) {
        int x = edit.position.x - key.x * 16, y = edit.position.y - key.y * 16,
            z = edit.position.z - key.z * 16;
        if (x >= -1 && x <= 16 && y >= -1 && y <= 16 && z >= -1 && z <= 16)
            halo[haloIndex(x, y, z)] = edit.value;
    }
    SubChunk result{key, {}, {}};
    const Vec3 origin{float(key.x * 16), float(key.y * 16), float(key.z * 16)};
    if (models) {
        meshModels(halo, origin, *models, result.vertices, 0);
        const auto count = unsigned(result.vertices.size());
        if (count)
            result.ranges.push_back({0, 0, count});
        std::vector<Vertex> leaves;
        meshModels(halo, origin, *models, leaves, 1);
        if (!leaves.empty()) {
            result.ranges.push_back({1, count, unsigned(leaves.size())});
            result.vertices.insert(result.vertices.end(), leaves.begin(), leaves.end());
        }
    } else
        meshSubChunk(halo, origin, result.vertices, &result.ranges);
    return result;
}
} // namespace
SubChunk generateSubChunk(const Terrain& terrain, ChunkKey key, const std::vector<BlockEdit>& edits,
                          const BlockModels* models) {
    return buildMesh(sampleColumns(terrain, key.x, key.z), key, edits, models,
                     terrain.dimension() != Dimension::End);
}
void generateColumn(Column& col, const Terrain& terrain, int cx, int cz,
                    const std::vector<BlockEdit>& edits, const BlockModels* models) {
    col.x = cx;
    col.z = cz;
    col.chunks.clear();
    const auto columns = sampleColumns(terrain, cx, cz);
    // Caves may expose any underground layer. Candidate processing happens once
    // per generation/edit job; zero meshes never enter the active CPU/GPU set.
    for (int sy = 0; sy < VerticalChunks; ++sy) {
        auto chunk =
            buildMesh(columns, {cx, sy, cz}, edits, models, terrain.dimension() != Dimension::End);
        if (!chunk.vertices.empty())
            col.chunks.push_back(std::move(chunk));
    }
}
Vec3 Camera::forward() const {
    return {std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch)};
}
Vec3 Camera::right() const {
    return {std::cos(yaw), 0, std::sin(yaw)};
}
Vec3 Camera::up() const {
    return cross(right(), forward());
}
bool Camera::visible(Vec3 low, Vec3 high) const {
    const Vec3 center = (low + high) * 0.5f, half = (high - low) * 0.5f, rel = center - eye;
    const Vec3 f = forward(), r = right(), u = up();
    const float ty = std::tan(fov * 0.5f), tx = ty * aspect;
    const Vec3 normals[6] = {f, f * -1, f * tx + r, f * tx - r, f * ty + u, f * ty - u};
    const float offsets[6] = {-nearPlane, farPlane, 0, 0, 0, 0};
    for (int i = 0; i < 6; ++i) {
        const auto n = normals[i];
        const float radius =
            std::abs(n.x) * half.x + std::abs(n.y) * half.y + std::abs(n.z) * half.z;
        if (dot(n, rel) + offsets[i] + radius < 0)
            return false;
    }
    return true;
}
void Player::spawn(const Terrain& terrain) {
    feet = {0.5f, float(terrain.surface(0, 0).height) + 0.02f, 0.5f};
    if (collides(terrain, *this, feet)) {
        bool found = false;
        for (int radius = 1; radius <= 8 && !found; ++radius)
            for (int z = -radius; z <= radius && !found; ++z)
                for (int x = -radius; x <= radius && !found; ++x) {
                    const Vec3 candidate{float(x) + 0.5f,
                                         float(terrain.surface(x, z).height) + 0.02f,
                                         float(z) + 0.5f};
                    if (!collides(terrain, *this, candidate)) {
                        feet = candidate;
                        found = true;
                    }
                }
        if (!found)
            feet.y = WorldHeight + 1;
    }
    verticalSpeed = 0;
    grounded = false;
    camera.eye = feet + Vec3{0, 1.62f, 0};
}
bool Aabb::overlaps(const Aabb& b) const {
    return minimum.x < b.maximum.x && maximum.x > b.minimum.x && minimum.y < b.maximum.y &&
           maximum.y > b.minimum.y && minimum.z < b.maximum.z && maximum.z > b.minimum.z;
}
Aabb PhysicsBody::bounds() const {
    return {feet - Vec3{radius, 0, radius}, feet + Vec3{radius, height, radius}};
}
bool collides(const BlockSource& blocks, const PhysicsBody& body, Vec3 p) {
    constexpr float epsilon = 0.0001f;
    if (std::abs(p.x) + body.radius > WorldLimit || std::abs(p.z) + body.radius > WorldLimit)
        return true;
    for (int z = int(std::floor(p.z - body.radius));
         z <= int(std::floor(p.z + body.radius - epsilon)); ++z)
        for (int x = int(std::floor(p.x - body.radius));
             x <= int(std::floor(p.x + body.radius - epsilon)); ++x)
            for (int y = int(std::floor(p.y)); y <= int(std::floor(p.y + body.height - epsilon));
                 ++y)
                if (blocks.block(x, y, z) != Block::Air)
                    return true;
    return false;
}
void moveBody(const BlockSource& blocks, PhysicsBody& body, Movement input, float yaw, float speed,
              float dt) {
    dt = std::clamp(dt, 0.0f, 0.05f);
    if (dt == 0)
        return;
    Vec3 direction = Vec3{std::sin(yaw), 0, -std::cos(yaw)} * input.forward +
                     Vec3{std::cos(yaw), 0, std::sin(yaw)} * input.strafe;
    if (dot(direction, direction) > 1)
        direction = normalized(direction);
    if (input.jump && body.grounded) {
        body.verticalSpeed = 7.5f;
        body.grounded = false;
    }
    body.verticalSpeed = std::max(body.verticalSpeed - 24.0f * dt, -32.0f);
    const Vec3 delta = direction * (speed * dt) + Vec3{0, body.verticalSpeed * dt, 0};
    const float largest = std::max({std::abs(delta.x), std::abs(delta.y), std::abs(delta.z)});
    const int steps = std::max(1, int(std::ceil(largest / 0.15f)));
    const Vec3 increment = delta * (1.0f / steps);
    body.grounded = false;
    bool verticalBlocked = false;
    for (int i = 0; i < steps; ++i) {
        Vec3 next = body.feet + Vec3{increment.x, 0, 0};
        if (!collides(blocks, body, next))
            body.feet = next;
        next = body.feet + Vec3{0, 0, increment.z};
        if (!collides(blocks, body, next))
            body.feet = next;
        if (verticalBlocked)
            continue;
        next = body.feet + Vec3{0, increment.y, 0};
        if (!collides(blocks, body, next))
            body.feet = next;
        else {
            if (increment.y < 0)
                body.grounded = true;
            body.verticalSpeed = 0;
            verticalBlocked = true;
        }
    }
}
void Player::step(const BlockSource& blocks, Movement input, float dt, bool crouch) {
    const float oldHeight = height;
    height = crouch ? 1.3f : 1.8f;
    if (height > oldHeight && collides(blocks, *this, feet))
        height = oldHeight;
    const bool low = height < 1.8f;
    moveBody(blocks, *this, input, camera.yaw, low ? 1.6f : 4.5f, dt);
    camera.eye = feet + Vec3{0, low ? 1.12f : 1.62f, 0};
}
} // namespace voxel

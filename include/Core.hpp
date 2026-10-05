#pragma once

#include "Content.hpp"
#include "GameTypes.hpp"
#include <array>
#include <cstdint>
#include <tuple>
#include <vector>

namespace voxel {
constexpr int ChunkSize = 16;
constexpr int WorldHeight = 64;
constexpr int VerticalChunks = WorldHeight / ChunkSize;
constexpr int RenderRadius = 2; // 5 x 5 columns, 100 independently culled sub-chunks.
constexpr float WorldLimit = 8192.0f;
constexpr float Pi = 3.14159265358979323846f;

struct Vec3 {
    float x{}, y{}, z{};
    Vec3 operator+(Vec3 b) const { return {x + b.x, y + b.y, z + b.z}; }
    Vec3 operator-(Vec3 b) const { return {x - b.x, y - b.y, z - b.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
};
float dot(Vec3 a, Vec3 b);
Vec3 cross(Vec3 a, Vec3 b);
Vec3 normalized(Vec3 v);
int floorDiv(int n, int divisor);

enum class Biome : std::uint8_t { Plains, Hills, Rocky };

class Perlin {
  public:
    explicit Perlin(std::uint32_t seed);
    float noise(float x, float y) const;
    float noise(float x, float y, float z) const;
    float fractal(float x, float y, int octaves = 4) const;

  private:
    std::array<std::uint8_t, 512> permutation_{};
};

struct Surface {
    int height;
    Biome biome;
};
class BlockSource {
  public:
    virtual ~BlockSource() = default;
    virtual Block block(int x, int y, int z) const = 0;
};

class Terrain : public BlockSource {
  public:
    explicit Terrain(std::uint32_t seed, unsigned version = 2,
                     Dimension dimension = Dimension::Overworld, WorldType type = WorldType::Normal)
        : height_(seed), climate_(seed ^ 0x9e3779b9u), caves_(seed ^ 0x85ebca6bu), seed_(seed),
          version_(version), dimension_(dimension), type_(type) {}
    Dimension dimension() const { return dimension_; }
    WorldType type() const { return type_; }
    std::uint32_t seed() const { return seed_; }
    unsigned version() const { return version_; }
    void sampleColumn(int x, int z, std::array<Block, WorldHeight>& out) const;
    Surface surface(int x, int z) const;
    static Block atHeight(Surface surface, int y);
    Block block(int x, int y, int z) const override;

  private:
    Perlin height_, climate_, caves_;
    std::uint32_t seed_;
    unsigned version_;
    Dimension dimension_;
    WorldType type_;
    Block sample(int x, int y, int z, Surface surface) const;
    Block vegetation(int x, int y, int z) const;
};

struct BlockPos {
    int x{}, y{}, z{};
    bool operator==(BlockPos b) const { return x == b.x && y == b.y && z == b.z; }
    bool operator<(BlockPos b) const { return std::tie(x, y, z) < std::tie(b.x, b.y, b.z); }
};
using ChunkKey = BlockPos;
ChunkKey chunkOf(BlockPos p);
struct BlockEdit {
    BlockPos position;
    Block value;
};

// Explicit normals support a moving sun without rebuilding meshes.
struct Vertex {
    float x, y, z, r, g, b, nx, ny, nz;
    float u{}, v{};
};
enum class Face : unsigned { East, West, Up, Down, South, North };
struct ModelFace {
    std::array<float, 4> uv{{0, 0, 1, 1}}; // u-left, v-bottom, u-right, v-top
    unsigned rotation{};
    bool present{}, cull{}, cutout{};
    Vec3 tint{1, 1, 1};
};
struct ModelElement {
    Vec3 from{}, to{1, 1, 1};
    std::array<ModelFace, 6> faces{};
};
struct BlockModel {
    std::vector<ModelElement> elements;
    bool occludes = true;
};
using BlockModels = std::array<BlockModel, BlockCount>;
struct MeshRange {
    unsigned material{}, first{}, count{};
};
struct SubChunk {
    ChunkKey key;
    std::vector<Vertex> vertices;
    std::vector<MeshRange> ranges;
};
struct Column {
    int x{}, z{};
    std::uint64_t revision{};
    std::vector<SubChunk> chunks; // Only nonempty generation results; no voxel arrays.
};
void generateColumn(Column& column, const Terrain& terrain, int x, int z,
                    const std::vector<BlockEdit>& edits = {}, const BlockModels* models = nullptr);
SubChunk generateSubChunk(const Terrain& terrain, ChunkKey key, const std::vector<BlockEdit>& edits,
                          const BlockModels* models = nullptr);
// Input includes a one-voxel halo. Only faces belonging to the inner 16^3 are emitted.
using Halo = std::array<Block, 18 * 18 * 18>;
void meshModels(const Halo& halo, Vec3 origin, const BlockModels& models, std::vector<Vertex>& out,
                int layer = -1);
void meshSubChunk(const Halo& halo, Vec3 origin, std::vector<Vertex>& output,
                  std::vector<MeshRange>* ranges = nullptr);

struct Camera {
    Vec3 eye{};
    float yaw{}, pitch{};
    float fov = 70.0f * Pi / 180.0f;
    float aspect = 320.0f / 240.0f;
    float nearPlane = 0.1f, farPlane = float(RenderRadius * ChunkSize);
    Vec3 forward() const;
    Vec3 right() const;
    Vec3 up() const;
    bool visible(Vec3 minimum, Vec3 maximum) const;
};

struct Movement {
    float forward{}, strafe{};
    bool jump{};
};
struct Aabb {
    Vec3 minimum, maximum;
    bool overlaps(const Aabb& other) const;
};
struct PhysicsBody {
    Vec3 feet{};
    float verticalSpeed{};
    bool grounded{};
    float radius = 0.3f, height = 1.8f;
    Aabb bounds() const;
};
bool collides(const BlockSource& blocks, const PhysicsBody& body, Vec3 position);
void moveBody(const BlockSource& blocks, PhysicsBody& body, Movement input, float yaw, float speed,
              float seconds);
class Player : public PhysicsBody {
  public:
    Camera camera{};
    void spawn(const Terrain& terrain);
    void step(const BlockSource& blocks, Movement input, float seconds, bool crouch = false);
};
} // namespace voxel

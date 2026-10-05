#include "Core.hpp"
namespace voxel {
void meshModels(const Halo& halo, Vec3 origin, const BlockModels& models, std::vector<Vertex>& out,
                int layer) {
    out.clear();
    const auto index = [](int x, int y, int z) { return (y + 1) * 324 + (z + 1) * 18 + x + 1; };
    const int direction[6][3] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                                 {0, -1, 0}, {0, 0, 1},  {0, 0, -1}};
    for (int y = 0; y < 16; ++y)
        for (int z = 0; z < 16; ++z)
            for (int x = 0; x < 16; ++x) {
                const auto block = halo[index(x, y, z)];
                if (block == Block::Air)
                    continue;
                const auto& model = models[unsigned(block)];
                for (const auto& element : model.elements)
                    for (unsigned face = 0; face < 6; ++face) {
                        const auto& f = element.faces[face];
                        if (!f.present || (layer >= 0 && int(f.cutout) != layer))
                            continue;
                        const auto& d = direction[face];
                        const auto neighbor = halo[index(x + d[0], y + d[1], z + d[2])];
                        if (f.cull && neighbor != Block::Air &&
                            (models[unsigned(neighbor)].occludes || neighbor == block))
                            continue;
                        const int axis = face / 2, u = (axis + 1) % 3, v = (axis + 2) % 3;
                        const bool positive = face % 2 == 0;
                        const float lo[] = {element.from.x, element.from.y, element.from.z};
                        const float hi[] = {element.to.x, element.to.y, element.to.z};
                        float p[3] = {float(x), float(y), float(z)};
                        p[axis] += positive ? hi[axis] : lo[axis];
                        p[u] += lo[u];
                        p[v] += lo[v];
                        Vec3 corners[4];
                        for (int i = 0; i < 4; ++i) {
                            float q[] = {p[0], p[1], p[2]};
                            if (i == 1 || i == 2)
                                q[u] += hi[u] - lo[u];
                            if (i == 2 || i == 3)
                                q[v] += hi[v] - lo[v];
                            corners[i] = origin + Vec3{q[0], q[1], q[2]};
                        }
                        // Minecraft face orientation, viewed from outside. V grows upwards on PICA.
                        const unsigned uvCorners[6][4] = {{1, 2, 3, 0}, {0, 3, 2, 1}, {3, 0, 1, 2},
                                                          {0, 3, 2, 1}, {0, 1, 2, 3}, {1, 0, 3, 2}};
                        const unsigned indices[2][6] = {{0, 2, 1, 0, 3, 2}, {0, 1, 2, 0, 2, 3}};
                        const float shade = face == 2   ? 1.f
                                            : face == 3 ? .5f
                                            : face < 2  ? .6f
                                                        : .8f;
                        for (unsigned k : indices[positive ? 1 : 0]) {
                            const auto q = corners[k];
                            const unsigned t = (uvCorners[face][k] + f.rotation / 90) % 4;
                            const float tu = t == 1 || t == 2 ? f.uv[2] : f.uv[0];
                            const float tv = t >= 2 ? f.uv[3] : f.uv[1];
                            out.push_back({q.x, q.y, q.z, f.tint.x * shade, f.tint.y * shade,
                                           f.tint.z * shade, float(d[0]), float(d[1]), float(d[2]),
                                           tu, tv});
                        }
                    }
            }
}
} // namespace voxel

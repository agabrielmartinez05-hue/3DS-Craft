#include "AssetCatalog.hpp"
#include "Error.hpp"
#include "Gui.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <regex>
#include <set>

namespace voxel {
namespace {
bool powerOfTwo(unsigned v) {
    return v >= 8 && v <= 1024 && (v & (v - 1)) == 0;
}
} // namespace
Image extractRegion(const Image& source, const TextureRegion& r) {
    if (!r.width || !r.height || r.x > source.width || r.width > source.width - r.x ||
        r.y > source.height || r.height > source.height - r.y ||
        source.rgba.size() != std::size_t(source.width) * source.height * 4)
        throw Error("PNG '%s': texture region is outside image or has unsupported dimensions",
                    r.file.c_str());
    Image image{r.width, r.height, {}};
    image.rgba.resize(std::size_t(r.width) * r.height * 4);
    for (unsigned y = 0; y < r.height; ++y)
        std::copy_n(source.rgba.data() + ((y + r.y) * source.width + r.x) * 4, r.width * 4,
                    image.rgba.data() + y * r.width * 4);
    return image;
}
std::vector<std::uint8_t> tileRgba8(const Image& image) {
    if (!powerOfTwo(image.width) || !powerOfTwo(image.height) ||
        image.rgba.size() != image.width * image.height * 4)
        throw Error("Invalid image for PICA RGBA8 conversion");
    std::vector<std::uint8_t> output(image.rgba.size());
    for (unsigned y = 0; y < image.height; ++y)
        for (unsigned x = 0; x < image.width; ++x) {
            const unsigned flipped = image.height - 1 - y;
            unsigned morton = 0;
            for (unsigned bit = 0; bit < 3; ++bit) {
                morton |= ((x >> bit) & 1) << (2 * bit);
                morton |= ((flipped >> bit) & 1) << (2 * bit + 1);
            }
            const unsigned destination =
                (((flipped / 8) * (image.width / 8) + x / 8) * 64 + morton) * 4;
            const auto* rgba = image.rgba.data() + (y * image.width + x) * 4;
            // GPU_RGBA8 uses ABGR byte order in memory; origin is bottom left.
            output[destination] = rgba[3];
            output[destination + 1] = rgba[2];
            output[destination + 2] = rgba[1];
            output[destination + 3] = rgba[0];
        }
    return output;
}
namespace {
using Json = nlohmann::json;
std::string resource(std::string id) {
    if (id.rfind("minecraft:", 0) == 0)
        id.erase(0, 10);
    if (!safeRelativePath(id))
        throw Error("Invalid Minecraft resource path '%s'", id.c_str());
    return id;
}
std::string modelPath(std::string id) {
    id = resource(id);
    if (id.rfind("block/", 0) != 0 && id.rfind("item/", 0) != 0)
        throw Error("Model path must use block/ or item/: '%s'", id.c_str());
    return id;
}
bool extension(std::string file, const char* expected) {
    const auto at = file.find_last_of('.');
    if (at == std::string::npos)
        return false;
    file = file.substr(at);
    for (auto& c : file)
        c = char(std::tolower(static_cast<unsigned char>(c)));
    return file == expected;
}
Json inherit(const std::string& id, const std::map<std::string, Json>& docs,
             std::set<std::string>& chain) {
    const auto it = docs.find(id);
    if (it == docs.end())
        throw Error("Missing model parent '%s'", id.c_str());
    if (chain.size() >= 16 || !chain.insert(id).second)
        throw Error("Model parent cycle/depth at '%s'", id.c_str());
    const auto& child = it->second;
    Json result = Json::object();
    if (child.contains("parent")) {
        const auto parent = resource(child.at("parent").get<std::string>());
        if (parent.rfind("builtin/", 0) != 0)
            result = inherit(modelPath(parent), docs, chain);
    }
    for (auto entry = child.begin(); entry != child.end(); ++entry) {
        if (entry.key() == "textures" && result.contains("textures")) {
            if (!entry.value().is_object())
                throw Error("textures must be an object");
            result["textures"].update(entry.value());
        } else
            result[entry.key()] = entry.value();
    }
    chain.erase(id);
    return result;
}
unsigned textureId(const Json& model, std::string ref, const std::map<std::string, unsigned>& ids) {
    std::set<std::string> chain;
    while (!ref.empty() && ref[0] == '#') {
        if (chain.size() >= 32 || !chain.insert(ref).second)
            throw Error("Texture variable cycle at '%s'", ref.c_str());
        ref = model.at("textures").at(ref.substr(1)).get<std::string>();
    }
    ref = resource(ref);
    auto it = ids.find(ref);
    if (it == ids.end())
        throw Error("Missing PNG texture '%s'", ref.c_str());
    return it->second;
}
float number(const Json& v) {
    if (!v.is_number())
        throw Error("Model coordinate must be numeric");
    const float n = v.get<float>();
    if (!std::isfinite(n) || n < 0 || n > 16)
        throw Error("Model coordinate must be within 0..16");
    return n;
}
Vec3 coordinate(const Json& v) {
    if (!v.is_array() || v.size() != 3)
        throw Error("Model from/to needs three coordinates");
    return {number(v[0]) / 16, number(v[1]) / 16, number(v[2]) / 16};
}
BlockModel compileModel(const Json& json, const std::map<std::string, unsigned>& ids,
                        const std::vector<TextureRegion>& regions, RenderLayer layer, Vec3 tint) {
    BlockModel model;
    const auto& elements = json.at("elements");
    if (!elements.is_array() || elements.empty() || elements.size() > 16)
        throw Error("Model requires 1..16 cuboid elements");
    const char* names[] = {"east", "west", "up", "down", "south", "north"};
    for (const auto& e : elements) {
        if (e.contains("rotation"))
            throw Error("Rotated model elements are not supported yet");
        ModelElement element;
        element.from = coordinate(e.at("from"));
        element.to = coordinate(e.at("to"));
        const float lo[] = {element.from.x, element.from.y, element.from.z};
        const float hi[] = {element.to.x, element.to.y, element.to.z};
        for (int axis = 0; axis < 3; ++axis)
            if (lo[axis] >= hi[axis])
                throw Error("Model from must precede to on every axis");
        const auto& faces = e.at("faces");
        if (!faces.is_object())
            throw Error("Model faces must be an object");
        for (auto it = faces.begin(); it != faces.end(); ++it)
            if (std::find(std::begin(names), std::end(names), it.key()) == std::end(names))
                throw Error("Unknown face '%s'", it.key().c_str());
        for (unsigned face = 0; face < 6; ++face) {
            if (!faces.contains(names[face]))
                continue;
            const auto& f = faces.at(names[face]);
            auto& out = element.faces[face];
            out.present = true;
            const auto& tile = regions.at(textureId(json, f.at("texture").get<std::string>(), ids));
            out.cutout = tile.transparent || layer == RenderLayer::Cutout;
            if (f.contains("tintindex"))
                out.tint = tint;
            // Minecraft defaults derive UVs from element dimensions, measured top-left.
            std::array<float, 4> uv;
            switch (face) {
            case 0:
                uv = {16 - hi[2] * 16, 16 - hi[1] * 16, 16 - lo[2] * 16, 16 - lo[1] * 16};
                break;
            case 1:
                uv = {lo[2] * 16, 16 - hi[1] * 16, hi[2] * 16, 16 - lo[1] * 16};
                break;
            case 2:
                uv = {lo[0] * 16, lo[2] * 16, hi[0] * 16, hi[2] * 16};
                break;
            case 3:
                uv = {lo[0] * 16, 16 - hi[2] * 16, hi[0] * 16, 16 - lo[2] * 16};
                break;
            case 4:
                uv = {lo[0] * 16, 16 - hi[1] * 16, hi[0] * 16, 16 - lo[1] * 16};
                break;
            default:
                uv = {16 - hi[0] * 16, 16 - hi[1] * 16, 16 - lo[0] * 16, 16 - lo[1] * 16};
            }
            if (f.contains("uv")) {
                const auto& values = f.at("uv");
                if (!values.is_array() || values.size() != 4)
                    throw Error("Face UV requires four coordinates");
                for (unsigned n = 0; n < 4; ++n)
                    uv[n] = number(values[n]);
            }
            const float du = tile.uv[2] - tile.uv[0], dv = tile.uv[3] - tile.uv[1];
            out.uv = {tile.uv[0] + du * uv[0] / 16, tile.uv[3] - dv * uv[3] / 16,
                      tile.uv[0] + du * uv[2] / 16, tile.uv[3] - dv * uv[1] / 16};
            if (f.contains("rotation")) {
                if (!f["rotation"].is_number_integer() ||
                    (f["rotation"].is_number_unsigned() &&
                     f["rotation"].get<std::uint64_t>() > 270))
                    throw Error("Face rotation must be 0,90,180,270");
                const int rotation = f["rotation"].get<int>();
                if (rotation < 0 || rotation > 270 || rotation % 90)
                    throw Error("Face rotation must be 0,90,180,270");
                out.rotation = unsigned(rotation);
            }
            if (f.contains("cullface")) {
                if (f["cullface"].get<std::string>() != names[face])
                    throw Error("cullface must match its face direction");
                // Internal faces (e.g. a slab top) cannot be culled by another voxel.
                out.cull = face % 2 == 0 ? hi[face / 2] == 1 : lo[face / 2] == 0;
            }
        }
        model.elements.push_back(element);
    }
    const auto& e = model.elements.front();
    model.occludes = layer == RenderLayer::Opaque && e.from.x == 0 && e.from.y == 0 &&
                     e.from.z == 0 && e.to.x == 1 && e.to.y == 1 && e.to.z == 1 &&
                     std::all_of(e.faces.begin(), e.faces.end(),
                                 [](const ModelFace& f) { return f.present && !f.cutout; });
    return model;
}
} // namespace
unsigned AssetCatalog::sprite(const std::string& id) const {
    const auto it = sprites_.find(id);
    if (it == sprites_.end())
        throw Error("Required sprite is not loaded: %s", id.c_str());
    return it->second;
}
void AssetCatalog::load(const std::string& root, const Progress& progress,
                        const TextureConsumer& consume) {
    AssetArchive archive;
    archive.preload(root, progress);
    load(archive, "", progress, consume);
}
void AssetCatalog::load(const AssetArchive& archive, const std::string& root,
                        const Progress& progress, const TextureConsumer& consume) {
    const auto prefix = root.empty() ? std::string() : root + "/";
    regions_.clear();
    documents_.clear();
    sprites_.clear();
    models_ = {};
    blocks_ = {};
    items_ = {};
    bytes_ = 0;
    atlasSize_ = 0;
    // The complete raw tree is already cached. Index recursively, but decode only
    // dependencies of registered gameplay models and the actual GUI being drawn.
    std::set<std::string> modelIndex;
    for (const auto& path : archive.list(prefix + "models/"))
        if (extension(path, ".json"))
            modelIndex.insert(path.substr(0, path.size() - 5));
    std::map<std::string, Json> raw;
    std::function<void(const std::string&, unsigned)> fetch = [&](const std::string& id,
                                                                  unsigned depth) {
        if (depth > 16)
            throw Error("Model parent cycle/depth at '%s'", id.c_str());
        if (raw.count(id))
            return;
        if (!modelIndex.count(id))
            throw Error("Missing model JSON: models/%s.json", id.c_str());
        auto json = archive.json(prefix + "models/" + id + ".json");
        if (!json.is_object())
            throw Error("Model '%s' must be an object", id.c_str());
        raw[id] = json;
        if (json.contains("parent")) {
            auto parent = resource(json.at("parent").get<std::string>());
            if (parent.rfind("builtin/", 0) != 0)
                fetch(modelPath(parent), depth + 1);
        }
    };
    const auto resolve = [&](const std::string& id) -> const Json& {
        auto it = documents_.find(id);
        if (it != documents_.end())
            return it->second;
        fetch(id, 0);
        std::set<std::string> chain;
        return documents_.emplace(id, inherit(id, raw, chain)).first->second;
    };
    std::map<std::string, std::string> declared;
    // Consume the repository's block/item/GUI/chest atlas source definitions.
    // Palette-derived armor trims are cached but never requested by this registry.
    for (const char* atlas : {"blocks", "items", "gui", "chests"}) {
        const auto path = prefix + "atlases/" + atlas + ".json";
        const auto json = archive.json(path);
        const auto& sources = json.at("sources");
        if (!sources.is_array())
            throw Error("Atlas '%s': sources must be an array", path.c_str());
        for (const auto& source : sources) {
            const auto type = resource(source.at("type").get<std::string>());
            if (type == "directory") {
                const auto directory = resource(source.at("source").get<std::string>()) + "/";
                const auto name = source.at("prefix").get<std::string>();
                for (const auto& file : archive.list(prefix + "textures/" + directory))
                    if (extension(file, ".png"))
                        declared[resource(name + file.substr(0, file.size() - 4))] =
                            directory + file;
            } else if (type == "single") {
                const auto id = resource(source.at("resource").get<std::string>());
                declared[resource(source.value("sprite", id))] = id + ".png";
            } else if (type == "paletted_permutations") {
                // No trim sprites in Content.hpp. Do not manufacture replacement pixels.
                if (!source.at("textures").is_array() || !source.at("permutations").is_object())
                    throw Error("Invalid palette atlas source '%s'", path.c_str());
            } else
                throw Error("Unsupported required atlas source '%s' in %s", type.c_str(),
                            path.c_str());
        }
    }
    std::map<std::string, std::string> wanted;
    const auto need = [&](std::string id, bool direct = false) {
        id = resource(id);
        auto at = declared.find(id);
        if (at == declared.end() && !direct)
            throw Error("Sprite '%s' is absent from repository atlas sources", id.c_str());
        const auto path = at == declared.end() ? id + ".png" : at->second;
        if (!archive.contains(prefix + "textures/" + path))
            throw Error("Missing PNG texture '%s'", path.c_str());
        wanted[id] = path;
    };
    const auto reference = [](const Json& model, std::string ref) {
        std::set<std::string> chain;
        while (!ref.empty() && ref.front() == '#') {
            if (chain.size() >= 32 || !chain.insert(ref).second)
                throw Error("Texture variable cycle '%s'", ref.c_str());
            ref = model.at("textures").at(ref.substr(1)).get<std::string>();
        }
        return resource(ref);
    };
    std::array<Json, BlockCount> blockJson;
    for (unsigned b = 1; b < BlockCount; ++b) {
        const auto id = std::string("block/") + BlockTypes[b].id;
        const auto& model = resolve(id);
        blockJson[b] = model;
        if (b == unsigned(Block::Lava)) {
            need(reference(model, "#particle"));
            // Fluids have a particle-only native model. Geometry uses the disk cube template.
            blockJson[b] = resolve("block/cube_all");
            blockJson[b]["textures"]["all"] = reference(model, "#particle");
        } else if (b == unsigned(Block::Chest)) {
            need("entity/chest/normal");
            continue; // Native entity-rendered block.
        }
        for (const auto& element : blockJson[b].at("elements"))
            for (const auto& face : element.at("faces"))
                need(reference(blockJson[b], face.at("texture").get<std::string>()));
    }
    std::array<std::string, ItemCount> itemSprites;
    for (unsigned i = 0; i < ItemCount; ++i) {
        const auto definition = archive.json(prefix + "items/" + ItemTypes[i].id + ".json");
        Json node = definition.at("model");
        for (unsigned depth = 0;; ++depth) {
            const auto kind = resource(node.at("type").get<std::string>());
            if (kind != "condition" && kind != "select")
                break;
            if (depth >= 16)
                throw Error("Item condition nesting too deep");
            node = kind == "condition" ? node.at("on_false") : node.at("fallback");
        }
        const auto type = resource(node.at("type").get<std::string>());
        if (type == "special") {
            resolve(modelPath(node.at("base").get<std::string>()));
            const auto special = resource(node.at("model").at("type").get<std::string>());
            if (special == "shield") {
                itemSprites[i] = "entity/shield/shield_base_nopattern";
                need(itemSprites[i], true);
            } else if (special == "chest") {
                itemSprites[i] = "entity/chest/normal";
                need(itemSprites[i]);
            } else
                throw Error("Unsupported special item '%s'", special.c_str());
        } else if (type == "model") {
            const auto& model = resolve(modelPath(node.at("model").get<std::string>()));
            const auto& textures = model.at("textures");
            itemSprites[i] =
                reference(model, textures.contains("layer0") ? "#layer0" : "#particle");
            need(itemSprites[i]);
        } else
            throw Error("Unsupported item definition '%s'", ItemTypes[i].id);
        items_[i].displayName = ItemTypes[i].name;
    }
    for (auto name : GuiSprites)
        need(name, std::string(name).rfind("gui/container/", 0) == 0);
    need("entity/villager/villager", true);
    // Pack one atlas containing real dependency pixels (GUI included), retaining raw bytes for all
    // files.
    std::vector<Image> images;
    std::map<std::string, unsigned> byFile;
    for (const auto& pair : wanted) {
        auto existing = byFile.find(pair.second);
        if (existing != byFile.end()) {
            sprites_[pair.first] = existing->second;
            continue;
        }
        if (progress)
            progress("Decode " + pair.second, unsigned(images.size()), unsigned(wanted.size()));
        auto image = archive.png(prefix + "textures/" + pair.second);
        const auto metaPath = prefix + "textures/" + pair.second + ".mcmeta";
        if (archive.contains(metaPath)) {
            const auto meta = archive.json(metaPath);
            if (meta.contains("animation")) {
                const auto& animation = meta.at("animation");
                unsigned w = animation.value("width", std::min(image.width, image.height)),
                         h = animation.value("height", w), frame = 0;
                if (animation.contains("frames") && !animation.at("frames").empty()) {
                    const auto& f = animation.at("frames")[0];
                    frame = f.is_object() ? f.at("index").get<unsigned>() : f.get<unsigned>();
                }
                if (!w || !h || w > image.width || h > image.height || image.width % w ||
                    image.height % h || frame >= image.width / w * (image.height / h))
                    throw Error("Invalid animation frame in '%s'", metaPath.c_str());
                image = extractRegion(image, {pair.second,
                                              frame % (image.width / w) * w,
                                              frame / (image.width / w) * h,
                                              w,
                                              h,
                                              {}});
            }
        }
        if (pair.second == "gui/container/inventory.png" ||
            pair.second == "gui/container/furnace.png")
            image = extractRegion(image, {pair.second, 0, 0, 176, 166, {}});
        if (pair.second == "gui/container/generic_54.png")
            image = extractRegion(image, {pair.second, 0, 0, 176, 222, {}});
        const unsigned id = unsigned(regions_.size());
        bool transparent = false;
        for (std::size_t n = 3; n < image.rgba.size(); n += 4)
            transparent |= image.rgba[n] < 128;
        regions_.push_back({pair.second, 0, 0, image.width, image.height, {}, transparent});
        images.push_back(std::move(image));
        sprites_[pair.first] = id;
        byFile[pair.second] = id;
    }
    std::vector<unsigned> order;
    for (unsigned i = 0; i < regions_.size(); ++i)
        order.push_back(i);
    std::stable_sort(order.begin(), order.end(), [&](unsigned a, unsigned b) {
        return regions_[a].height > regions_[b].height;
    });
    for (unsigned side = 256; side <= MaxAtlasSize; side *= 2) {
        // A bounded skyline fills space beside/below native GUI panels. The old
        // row packer wasted enough space to quadruple this pack's VRAM footprint.
        std::vector<unsigned> skyline(side, 0);
        bool fit = true;
        for (auto id : order) {
            auto& r = regions_[id];
            const unsigned width = r.width + 2, height = r.height + 2;
            unsigned bestX = 0, bestY = side + 1;
            if (width <= side && height <= side)
                for (unsigned x = 0; x + width <= side; ++x) {
                    unsigned y = 0;
                    for (unsigned i = x; i < x + width; ++i) {
                        y = std::max(y, skyline[i]);
                        if (y >= bestY)
                            break;
                    }
                    if (y < bestY && y + height <= side) {
                        bestX = x;
                        bestY = y;
                    }
                }
            if (bestY > side) {
                fit = false;
                break;
            }
            r.x = bestX + 1;
            r.y = bestY + 1;
            std::fill(skyline.begin() + bestX, skyline.begin() + bestX + width, bestY + height);
        }
        if (fit) {
            atlasSize_ = side;
            break;
        }
    }
    if (!atlasSize_)
        throw Error("Required repository textures do not fit the 1024x1024 VRAM atlas");
    Image atlas{atlasSize_, atlasSize_, std::vector<std::uint8_t>(atlasSize_ * atlasSize_ * 4)};
    for (unsigned id = 0; id < regions_.size(); ++id) {
        auto& r = regions_[id];
        const auto& src = images[id];
        for (int y = -1; y <= int(r.height); ++y)
            for (int x = -1; x <= int(r.width); ++x) {
                auto from = (std::clamp(y, 0, int(r.height) - 1) * r.width +
                             std::clamp(x, 0, int(r.width) - 1)) *
                            4;
                auto to = ((r.y + y) * atlasSize_ + r.x + x) * 4;
                std::copy_n(src.rgba.data() + from, 4, atlas.rgba.data() + to);
            }
        float side = float(atlasSize_);
        r.uv = {(r.x + .5f) / side, 1 - (r.y + r.height - .5f) / side, (r.x + r.width - .5f) / side,
                1 - (r.y + .5f) / side};
    }
    const auto mapColor = [&](const char* name) {
        auto map = archive.png(prefix + "textures/colormap/" + name + ".png");
        auto p = (((map.height - 1) / 2) * map.width + (map.width - 1) / 2) * 4;
        return Vec3{map.rgba[p] / 255.f, map.rgba[p + 1] / 255.f, map.rgba[p + 2] / 255.f};
    };
    for (unsigned b = 1; b < BlockCount; ++b) {
        if (b == unsigned(Block::Chest)) {
            const auto& r = regions_.at(sprite("entity/chest/normal"));
            ModelElement box;
            box.from = {1.f / 16, 0, 1.f / 16};
            box.to = {15.f / 16, 14.f / 16, 15.f / 16};
            // Native chest's 64x64 entity skin: body side/front and lid top rectangles.
            const unsigned rect[6][4] = {{0, 33, 14, 14},  {28, 33, 14, 14}, {14, 0, 14, 14},
                                         {28, 19, 14, 14}, {42, 33, 14, 14}, {14, 33, 14, 14}};
            for (unsigned f = 0; f < 6; ++f) {
                auto& out = box.faces[f];
                out.present = true;
                const auto* c = rect[f];
                const float u = r.uv[2] - r.uv[0], v = r.uv[3] - r.uv[1];
                out.uv = {r.uv[0] + u * c[0] / 64, r.uv[3] - v * (c[1] + c[3]) / 64,
                          r.uv[0] + u * (c[0] + c[2]) / 64, r.uv[3] - v * c[1] / 64};
            }
            models_[b].elements.push_back(box);
            models_[b].occludes = false;
            blocks_[b].opensInventory = true;
            continue;
        }
        const auto tint = b == unsigned(Block::Grass)         ? mapColor("grass")
                          : b == unsigned(Block::BirchLeaves) ? Vec3{.502f, .655f, .333f}
                                                              : mapColor("foliage");
        models_[b] = compileModel(blockJson[b], sprites_, regions_, BlockTypes[b].layer, tint);
        blocks_[b].breakable = b != unsigned(Block::Lava);
    }
    for (unsigned i = 0; i < ItemCount; ++i)
        items_[i].texture = sprite(itemSprites[i]);
    if (consume)
        consume(0, atlas);
    bytes_ = atlas.rgba.size();
    if (progress)
        progress("Repository textures ready", 1, 1);
}
} // namespace voxel

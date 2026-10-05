#include "Renderer.hpp"
#include "Error.hpp"
#include "World.hpp"
#include "world_shbin.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace voxel {
namespace {
constexpr u32 TransferFlags =
    GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
    GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO);
std::array<char, 1600> wrapError(const char* message) {
    std::array<char, 1600> wrapped{};
    std::size_t at = 0;
    int column = 0;
    for (std::size_t i = 0; message[i] && at + 2 < wrapped.size(); ++i) {
        if (column == 45) {
            wrapped[at++] = '\n';
            column = 0;
        }
        wrapped[at++] = message[i];
        column = message[i] == '\n' ? 0 : column + 1;
    }
    return wrapped;
}
void prepareUi() {
    C2D_Prepare();
    C3D_AlphaTest(false, GPU_ALWAYS, 0);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_ONE,
                   GPU_ONE_MINUS_SRC_ALPHA);
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
}
} // namespace
Frame::Frame() {
    if (!C3D_FrameBegin(C3D_FRAME_SYNCDRAW))
        throw Error("Citro3D: unable to begin frame");
}
Frame::~Frame() {
    C3D_FrameEnd(0);
}
void Renderer::initialize() {
    gfxInitDefault();
    gfx_ = true;
    gfxSet3D(false);
    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE * 2))
        throw Error("Citro3D initialization failed (command buffer or GPU memory allocation)");
    c3d_ = true;
    if (!C2D_Init(2048))
        throw Error("Citro2D initialization failed (UI buffers or shader allocation)");
    c2d_ = true;
    // Reserve error UI resources before terrain allocations.
    bottom_ = C3D_RenderTargetCreate(240, 320, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    if (!bottom_)
        throw Error("Unable to allocate bottom-screen world/UI color and depth target");
    C3D_RenderTargetSetOutput(bottom_, GFX_BOTTOM, GFX_LEFT, TransferFlags);
    text_ = C2D_TextBufNew(2048);
    if (!text_)
        throw Error("Unable to allocate Citro2D text buffer");
    animalVertices_ =
        static_cast<Vertex*>(linearAlloc(2 * AnimalSystem::MaxModelVertices * sizeof(Vertex)));
    if (!animalVertices_)
        throw Error("Unable to allocate animal vertex buffer");
    animalStaging_.reserve(AnimalSystem::MaxModelVertices);
    shader_ =
        DVLB_ParseFile(reinterpret_cast<u32*>(const_cast<u8*>(world_shbin)), world_shbin_size);
    if (!shader_)
        throw Error("Unable to parse embedded world vertex shader");
    if (R_FAILED(shaderProgramInit(&program_)))
        throw Error("World shader program initialization failed");
    programReady_ = true;
    if (R_FAILED(shaderProgramSetVsh(&program_, &shader_->DVLE[0])))
        throw Error("Unable to bind world vertex shader");
    projection_ = shaderInstanceGetUniformLocation(program_.vertexShader, "projection");
    view_ = shaderInstanceGetUniformLocation(program_.vertexShader, "view");
    sun_ = shaderInstanceGetUniformLocation(program_.vertexShader, "sunDirection");
    lighting_ = shaderInstanceGetUniformLocation(program_.vertexShader, "lighting");
    if (projection_ < 0 || view_ < 0 || sun_ < 0 || lighting_ < 0)
        throw Error("World shader is missing camera/lighting uniforms");
    AttrInfo_Init(&attributes_);
    AttrInfo_AddLoader(&attributes_, 0, GPU_FLOAT, 3);
    AttrInfo_AddLoader(&attributes_, 1, GPU_FLOAT, 3);
    AttrInfo_AddLoader(&attributes_, 2, GPU_FLOAT, 3);
    AttrInfo_AddLoader(&attributes_, 3, GPU_FLOAT, 2);
}
void Renderer::label(const char* message, float x, float y, float scale, u32 color) {
    C2D_Text text{};
    C2D_TextParse(&text, text_, message);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor | C2D_WordWrap, x, y, 0, scale, scale, color, 300.0f);
}
void Renderer::draw(const World& world, const Camera& camera, const AnimalSystem& animals,
                    const DayNight& cycle, const Inventory& inventory,
                    const PerformanceMetrics& metrics, const char* message, const GpuAssets& assets,
                    bool inventoryOpen, unsigned inventoryCursor, const Survival& survival,
                    GameMode mode, Dimension dimension, bool debugMesh,
                    const std::array<unsigned, ItemCount>* chest, unsigned inventoryPage) {
    auto light = cycle.light();
    if (dimension == Dimension::Nether) {
        light.sky = {0.22f, 0.025f, 0.02f};
        light.ambient = 0.6f;
        light.diffuse = 0.25f;
    }
    if (dimension == Dimension::End) {
        light.sky = {0.06f, 0.025f, 0.09f};
        light.ambient = 0.65f;
        light.diffuse = 0.2f;
    }
    const auto byte = [](float c) { return static_cast<u32>(std::clamp(c, 0.0f, 1.0f) * 255); };
    const u32 clear =
        (byte(light.sky.x) << 24) | (byte(light.sky.y) << 16) | (byte(light.sky.z) << 8) | 255;
    C3D_RenderTargetClear(bottom_, C3D_CLEAR_ALL, clear, 0);
    C3D_FrameDrawOn(bottom_);
    C3D_BindProgram(&program_);
    C3D_SetAttrInfo(&attributes_);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);
    C3D_StencilTest(false, GPU_ALWAYS, 0, 0xff, 0);
    C3D_TexEnvBufUpdate(C3D_Both, 0);
    C3D_CullFace(debugMesh ? GPU_CULL_NONE : GPU_CULL_BACK_CCW);
    C3D_AlphaTest(false, GPU_ALWAYS, 0);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_RGB, debugMesh ? GPU_PRIMARY_COLOR : GPU_TEXTURE0, GPU_PRIMARY_COLOR,
                  GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_RGB, debugMesh ? GPU_REPLACE : GPU_MODULATE);
    C3D_TexEnvSrc(env, C3D_Alpha, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
    for (int i = 1; i < 6; ++i)
        C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_Mtx projection, view;
    Mtx_PerspTilt(&projection, camera.fov, camera.aspect, camera.nearPlane, camera.farPlane, false);
    const auto at = camera.eye + camera.forward();
    Mtx_LookAt(&view, FVec3_New(camera.eye.x, camera.eye.y, camera.eye.z),
               FVec3_New(at.x, at.y, at.z), FVec3_New(0, 1, 0), false);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, projection_, &projection);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, view_, &view);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, sun_, light.sunDirection.x, light.sunDirection.y,
                  light.sunDirection.z, 0);
    const float daylight = std::clamp(light.ambient + light.diffuse, .18f, 1.f);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, lighting_, daylight * .9f, daylight * .1f, 0, 0);
    const auto drawBuffer = [](Vertex* vertices, int first, int count) {
        C3D_BufInfo buffer;
        BufInfo_Init(&buffer);
        if (BufInfo_Add(&buffer, vertices, sizeof(Vertex), 4, 0x3210) < 0)
            throw Error("Citro3D rejected vertex buffer");
        C3D_SetBufInfo(&buffer);
        C3D_DrawArrays(GPU_TRIANGLES, first, count);
    };
    C3D_TexBind(0, assets.texture(0));
    unsigned visible = 0;
    // This collection contains only active meshes. There are no empty slots to visit.
    for (const auto& entry : world.chunks()) {
        const auto key = entry.first;
        const Vec3 low{float(key.x * 16), float(key.y * 16), float(key.z * 16)};
        if (!debugMesh && !camera.visible(low, low + Vec3{16, 16, 16}))
            continue;
        for (const auto& range : entry.second->ranges) {
            const bool cutout = range.material == 1 && !debugMesh;
            C3D_AlphaTest(cutout, GPU_GREATER, 127);
            env = C3D_GetTexEnv(0);
            C3D_TexEnvSrc(env, C3D_Alpha, cutout ? GPU_TEXTURE0 : GPU_PRIMARY_COLOR,
                          GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
            drawBuffer(entry.second->vertices, range.first, range.count);
        }
        ++visible;
    }
    C3D_AlphaTest(false, GPU_ALWAYS, 0);
    // Animals retain colored box models; only world blocks use texture modulation.
    env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    animals.appendVisible(camera, animalStaging_);
    if (animalStaging_.size() > AnimalSystem::MaxModelVertices)
        throw Error("Animal mesh budget exceeded");
    if (!animalStaging_.empty()) {
        std::memcpy(animalVertices_, animalStaging_.data(), animalStaging_.size() * sizeof(Vertex));
        drawBuffer(animalVertices_, 0, static_cast<int>(animalStaging_.size()));
    }
    // Villagers use the repository entity skin in the same world atlas.
    animals.appendVisible(camera, animalStaging_, true);
    if (!animalStaging_.empty()) {
        C3D_TexBind(0, assets.texture(0));
        C3D_TexEnvSrc(env, C3D_RGB, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env, C3D_RGB, GPU_MODULATE);
        C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
        C3D_AlphaTest(true, GPU_GREATER, 127);
        // Distinct buffer offset: queued colored draws must retain their vertices.
        auto* destination = animalVertices_ + AnimalSystem::MaxModelVertices;
        std::memcpy(destination, animalStaging_.data(), animalStaging_.size() * sizeof(Vertex));
        drawBuffer(destination, 0, int(animalStaging_.size()));
    }
    // Citro2D copies sprite coordinates into its own vertices; every image points
    // at the decoded repository atlas, never generated replacement artwork.
    C2D_TextBufClear(text_);
    prepareUi();
    C2D_SceneBegin(bottom_);
    const u32 white = C2D_Color32(255, 255, 255, 255);
    const auto imageAt = [&](C2D_Image image, float x, float y, float scale = 1.f) {
        C2D_DrawImageAt(image, x, y, 0, nullptr, scale, scale);
    };
    const auto cropAt = [&](C2D_Image image, unsigned sx, unsigned sy, unsigned w, unsigned h,
                            float x, float y) {
        auto view = *image.subtex;
        float du = (view.right - view.left) / view.width,
              dv = (view.top - view.bottom) / view.height;
        view.left += sx * du;
        view.right = view.left + w * du;
        view.top -= sy * dv;
        view.bottom = view.top - h * dv;
        view.width = w;
        view.height = h;
        C2D_DrawImageAt({image.tex, &view}, x, y, 0);
    };
    char line[256];
    std::snprintf(line, sizeof(line), "FPS %.1f | %u/%u chunks | %u verts", metrics.fps(), visible,
                  unsigned(world.chunks().size()), world.vertexCount());
    label(line, 3, 2, .32f, white);
    std::snprintf(line, sizeof(line), "%s | Hit %.0f%%", dimensionName(dimension),
                  100 * survival.charge(inventory.selected()));
    label(line, 3, 14, .32f, white);
    imageAt(assets.gui(GuiSprite::Crosshair), 152.5f, 112.5f);
    imageAt(assets.gui(GuiSprite::Hotbar), HotbarX, HotbarY);
    auto found = std::find(HotbarItems.begin(), HotbarItems.end(), inventory.selected());
    unsigned index = found == HotbarItems.end() ? 0 : unsigned(found - HotbarItems.begin());
    unsigned first = index / 9 * 9, slot = index % 9;
    imageAt(assets.gui(GuiSprite::Selection), HotbarX - 1 + slot * 20, HotbarY - 1);
    for (unsigned n = 0; n < 9 && first + n < HotbarItems.size(); ++n) {
        auto item = HotbarItems[first + n];
        if (n == slot && found == HotbarItems.end())
            item = inventory.selected();
        if (!inventory.count(item))
            continue;
        auto icon = assets.icon(item);
        C2D_DrawImageAt(icon, HotbarX + 3 + n * 20, HotbarY + 3, 0, nullptr,
                        16.f / icon.subtex->width, 16.f / icon.subtex->height);
        std::snprintf(line, sizeof(line), "%u", inventory.count(item));
        label(line, HotbarX + 8 + n * 20, HotbarY + 12, .28f, white);
    }
    if (mode == GameMode::Survival)
        for (unsigned n = 0; n < 10; ++n) {
            float x = HotbarX + n * 8;
            imageAt(assets.gui(GuiSprite::HeartEmpty), x, HotbarY - 12);
            if (survival.health > n * 2)
                imageAt(assets.gui(survival.health >= n * 2 + 2 ? GuiSprite::HeartFull
                                                                : GuiSprite::HeartHalf),
                        x, HotbarY - 12);
            x = HotbarX + 173 - n * 8;
            imageAt(assets.gui(GuiSprite::FoodEmpty), x, HotbarY - 12);
            if (survival.hunger > n * 2)
                imageAt(assets.gui(survival.hunger >= n * 2 + 2 ? GuiSprite::FoodFull
                                                                : GuiSprite::FoodHalf),
                        x, HotbarY - 12);
        }
    if (survival.offhand != Item::Count) {
        imageAt(assets.gui(GuiSprite::Offhand), HotbarX - 30, HotbarY - 1);
        auto icon = assets.icon(survival.offhand);
        C2D_DrawImageAt(icon, HotbarX - 22, HotbarY + 3, 0, nullptr, 16.f / icon.subtex->width,
                        16.f / icon.subtex->height);
    }
    if (mode == GameMode::Survival && survival.armorPoints())
        for (unsigned n = 0; n < 10; ++n) {
            const float x = HotbarX + n * 8, y = HotbarY - 22;
            imageAt(assets.gui(GuiSprite::ArmorEmpty), x, y);
            if (survival.armorPoints() > n * 2)
                imageAt(assets.gui(survival.armorPoints() >= n * 2 + 2 ? GuiSprite::ArmorFull
                                                                       : GuiSprite::ArmorHalf),
                        x, y);
        }
    label(message, 4, 166, .32f, white);
    if (inventoryOpen) {
        const float x = 72, y = 30;
        if (chest) {
            // Compose three chest rows and the native player's inventory section.
            cropAt(assets.gui(GuiSprite::Chest), 0, 0, 176, 71, x, y);
            cropAt(assets.gui(GuiSprite::Chest), 0, 125, 176, 97, x, y + 71);
            label("Chest", x + 8, y + 4, .4f, C2D_Color32(55, 55, 55, 255));
        } else {
            cropAt(assets.gui(GuiSprite::Inventory), 0, 0, 176, 166, x, y);
            label("SELECT: Craft", x + 84, y + 10, .35f, C2D_Color32(55, 55, 55, 255));
        }
        for (unsigned pane = 0; pane < (chest ? 2u : 1u); ++pane)
            for (unsigned i = 0; i < InventoryPageSize; ++i) {
                const auto item = inventoryPageItem(inventoryPage, i);
                if (item == Item::Count)
                    continue;
                float px = x + 8 + (i % 9) * 18,
                      py = y + (chest ? (pane == 0 ? 18 : 84) : 84) + (i / 9) * 18;
                auto amount = chest && pane == 0 ? (*chest)[unsigned(item)] : inventory.count(item);
                if (amount) {
                    auto icon = assets.icon(item);
                    C2D_DrawImageAt(icon, px, py, 0, nullptr, 16.f / icon.subtex->width,
                                    16.f / icon.subtex->height);
                    std::snprintf(line, sizeof(line), "%u", amount);
                    label(line, px + 5, py + 8, .28f, white);
                }
                if (inventoryCursor == pane * 27 + i) {
                    auto icon = assets.gui(GuiSprite::Selection);
                    C2D_DrawImageAt(icon, px - 2, py - 2, 0, nullptr, 20.f / icon.subtex->width,
                                    20.f / icon.subtex->height);
                }
            }
        auto selected = inventoryPageItem(inventoryPage, inventoryCursor);
        std::snprintf(line, sizeof(line), "%u/%u %s", inventoryPage + 1, InventoryPages,
                      Inventory::name(selected));
        label(line, x + 8, y + 148, .35f, C2D_Color32(55, 55, 55, 255));
        label(chest ? "L one | R stack | ZL/ZR page | X/Y close"
                    : "L select | ZL/ZR page | X/Y close",
              15, 204, .38f, white);
    }
    std::snprintf(line, sizeof(line), "RAM %.1f / %.1f MiB | SELECT actions",
                  metrics.liveBytes() / 1048576., metrics.reservedBytes() / 1048576.);
    label(line, 3, 228, .33f, white);
    if (survival.dead()) {
        C2D_DrawRectSolid(12, 73, 0, 296, 94, C2D_Color32(72, 8, 12, 220));
        label("You died", 85, 83, .8f, white);
        label("B respawn | START save and leave", 28, 130, .43f, white);
    }
    C2D_Flush();
}
void Renderer::menu(const std::string& title, const std::vector<MenuRow>& rows, unsigned selected,
                    const std::string& subtitle, const char* footer) {
    Frame frame;
    C2D_TargetClear(bottom_, C2D_Color32(17, 28, 37, 255));
    prepareUi();
    C2D_SceneBegin(bottom_);
    C2D_TextBufClear(text_);
    const auto white = C2D_Color32(245, 248, 255, 255);
    label(title.c_str(), 12, 10, 0.65f, white);
    label(subtitle.c_str(), 12, 35, 0.38f, C2D_Color32(170, 200, 210, 255));
    const unsigned first = (selected / 5) * 5;
    for (unsigned i = first; i < rows.size() && i < first + 5; ++i) {
        const float y = 65 + float(i - first) * 28;
        if (i == selected)
            C2D_DrawRectSolid(8, y - 2, 0, 304, 26, C2D_Color32(46, 86, 106, 255));
        label(rows[i].label.c_str(), 14, y, 0.46f,
              rows[i].enabled ? white : C2D_Color32(220, 120, 110, 255));
    }
    label(footer, 10, 222, 0.36f, white);
    C2D_Flush();
}
void Renderer::furnace(const Furnace& furnace, const GpuAssets& assets, Item selectedItem,
                       unsigned action, const char* message) {
    Frame frame;
    C2D_TargetClear(bottom_, C2D_Color32(17, 28, 37, 255));
    prepareUi();
    C2D_SceneBegin(bottom_);
    C2D_TextBufClear(text_);
    auto panel = assets.gui(GuiSprite::Furnace);
    C2D_DrawImageAt(panel, 72, 10, 0);
    const u32 dark = C2D_Color32(55, 55, 55, 255), white = C2D_Color32(255, 255, 255, 255);
    label("Furnace", 80, 14, .4f, dark);
    const auto icon = [&](Item item, unsigned count, float x, float y) {
        if (!count || item == Item::Count)
            return;
        auto image = assets.icon(item);
        C2D_DrawImageAt(image, x, y, 0, nullptr, 16.f / image.subtex->width,
                        16.f / image.subtex->height);
        char text[16];
        std::snprintf(text, sizeof(text), "%u", count);
        label(text, x + 5, y + 8, .3f, white);
    };
    icon(furnace.input, furnace.inputCount, 128, 27);
    icon(furnace.fuel, furnace.fuelCount, 128, 63);
    icon(furnace.output, furnace.outputCount, 188, 45);
    if (furnace.burn > 0)
        C2D_DrawImageAt(assets.gui(GuiSprite::FurnaceBurn), 128, 46, 0);
    if (furnace.progress > 0) {
        auto image = assets.gui(GuiSprite::FurnaceCook);
        auto view = *image.subtex;
        const float ratio = furnace.progress / 10;
        view.width = std::max(1u, unsigned(view.width * ratio));
        view.right = view.left + (view.right - view.left) * ratio;
        C2D_DrawImageAt({image.tex, &view}, 151, 45, 0);
    }
    label("Selected item:", 80, 98, .4f, dark);
    label(Inventory::name(selectedItem), 80, 116, .4f, dark);
    static const char* actions[] = {"Add one input",  "Add input stack", "Add one fuel",
                                    "Add fuel stack", "Collect output",  "Take input",
                                    "Take fuel",      "Resume"};
    char text[128];
    std::snprintf(text, sizeof(text), "%u/8  %s", action + 1, actions[action % 8]);
    label(text, 15, 180, .48f, white);
    label(message, 15, 202, .35f, white);
    label("D-pad action | L confirm | R back", 15, 223, .35f, white);
    C2D_Flush();
}
void Renderer::keyboard(const TextEditor& editor, unsigned selected, const char* title) {
    Frame frame;
    C2D_TargetClear(bottom_, C2D_Color32(17, 28, 37, 255));
    prepareUi();
    C2D_SceneBegin(bottom_);
    C2D_TextBufClear(text_);
    const auto white = C2D_Color32(255, 255, 255, 255);
    label(title, 10, 8, 0.6f, white);
    label(editor.text().c_str(), 10, 34, 0.48f, white);
    for (unsigned i = 0; i < editor.keys().size(); ++i) {
        const float x = 10 + float(i % 10) * 30, y = 70 + float(i / 10) * 28;
        C2D_DrawRectSolid(x, y, 0, 28, 25,
                          i == selected ? C2D_Color32(70, 130, 150, 255)
                                        : C2D_Color32(40, 58, 70, 255));
        label(editor.keys()[i].c_str(), x + 2, y + 3, 0.37f, white);
    }
    label("Touch keys or D-pad + L | R cancel", 10, 221, 0.4f, white);
    C2D_Flush();
}
void Renderer::loading(const std::string& message, unsigned done, unsigned total) {
    Frame frame;
    C2D_TargetClear(bottom_, C2D_Color32(17, 28, 37, 255));
    prepareUi();
    C2D_SceneBegin(bottom_);
    C2D_TextBufClear(text_);
    label("Loading", 12, 18, 0.7f, C2D_Color32(255, 255, 255, 255));
    label(message.c_str(), 12, 60, 0.42f, C2D_Color32(225, 240, 240, 255));
    C2D_DrawRectSolid(12, 184, 0, 296, 12, C2D_Color32(40, 60, 70, 255));
    if (total)
        C2D_DrawRectSolid(12, 184, 0, 296 * std::min(1.0f, float(done) / total), 12,
                          C2D_Color32(100, 220, 150, 255));
    C2D_Flush();
}
void Renderer::promptError(const char* message) {
    const auto wrapped = wrapError(message);
    float scroll = 0;
    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & (KEY_L | KEY_R | KEY_B))
            return;
        if (hidKeysHeld() & KEY_DDOWN)
            scroll = std::min(1200.0f, scroll + 3);
        if (hidKeysHeld() & KEY_DUP)
            scroll = std::max(0.0f, scroll - 3);
        Frame frame;
        C2D_TargetClear(bottom_, C2D_Color32(40, 15, 20, 255));
        prepareUi();
        C2D_SceneBegin(bottom_);
        C2D_TextBufClear(text_);
        label("Unable to complete action", 8, 8 - scroll, 0.55f, C2D_Color32(255, 180, 180, 255));
        label(wrapped.data(), 8, 36 - scroll, 0.43f, C2D_Color32(255, 255, 255, 255));
        label("D-pad scroll | L/R return", 8, 223, 0.38f, C2D_Color32(255, 200, 200, 255));
        C2D_Flush();
    }
}
void Renderer::errorScreen(const char* message) noexcept {
    // C2D itself needs C3D. If either initialization failed, a framebuffer console is
    // the only independent fallback. Never retry allocations in an OOM handler.
    const bool useC2D = c3d_ && c2d_ && bottom_ && text_;
    if (!gfx_) {
        gfxInitDefault();
        gfx_ = true;
    }
    if (!useC2D) {
        shutdownGpu();
        consoleInit(GFX_BOTTOM, nullptr);
        std::printf("3DS Craft error\n\n%s\n\nSTART: exit\n", message);
    }
    float scroll = 0;
    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START)
            break;
        if (useC2D && C3D_FrameBegin(C3D_FRAME_SYNCDRAW)) {
            if (hidKeysHeld() & KEY_DDOWN)
                scroll += 3;
            if (hidKeysHeld() & KEY_DUP)
                scroll = scroll > 3 ? scroll - 3 : 0;
            if (scroll > 1200)
                scroll = 1200;
            C2D_TargetClear(bottom_, C2D_Color32(38, 14, 20, 255));
            prepareUi();
            C2D_SceneBegin(bottom_);
            C2D_TextBufClear(text_);
            // Fixed-width wrapping handles long filenames/tokens as well as prose.
            const auto wrapped = wrapError(message);
            label("3DS Craft error | D-pad scroll | START exit", 8, 8 - scroll, 0.42f,
                  C2D_Color32(255, 190, 190, 255));
            label(wrapped.data(), 8, 36 - scroll, 0.48f, C2D_Color32(255, 255, 255, 255));
            C2D_Flush();
            C3D_FrameEnd(0);
        } else {
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();
        }
    }
}
void Renderer::shutdownGpu() noexcept {
    if (c3d_ && C3D_FrameBegin(0))
        C3D_FrameEnd(0);
    if (programReady_) {
        shaderProgramFree(&program_);
        programReady_ = false;
    }
    if (shader_) {
        DVLB_Free(shader_);
        shader_ = nullptr;
    }
    if (text_) {
        C2D_TextBufDelete(text_);
        text_ = nullptr;
    }
    if (animalVertices_) {
        linearFree(animalVertices_);
        animalVertices_ = nullptr;
    }
    if (bottom_) {
        C3D_RenderTargetDelete(bottom_);
        bottom_ = nullptr;
    }
    if (c2d_) {
        C2D_Fini();
        c2d_ = false;
    }
    if (c3d_) {
        C3D_Fini();
        c3d_ = false;
    }
}
Renderer::~Renderer() {
    shutdownGpu();
    if (gfx_)
        gfxExit();
}
} // namespace voxel

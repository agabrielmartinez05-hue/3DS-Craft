# Repository asset contract

Copy the repository's **`assets/`** directory to **`sdmc:/assets/`** without renaming
folders. Both launch formats use the same external files:

```text
assets/minecraft/
  models/block/       Native block models and inherited parent templates
  models/item/        Native item models
  items/             Modern item-definition JSONs
  atlases/           blocks.json, items.json, gui.json, chests.json
  textures/block/    Terrain PNGs (nested paths supported)
  textures/item/     Item PNGs
  textures/gui/      HUD sprites and container sheets
  textures/entity/   Chest, shield and villager skins
  textures/colormap/ Grass and foliage tint data
```

`block/stone` resolves directly to `models/block/stone.json`. **There is no plural
`models/blocks` translation**, generated texture fallback, embedded replacement
model document or automatic substitute for a missing required resource.

## Startup and RAM

`AssetArchive` recursively caches all files and directory records before the main
menu. The upstream tree at revision `8ad31f2d9e3accd260e3ad9a0fe4e09b906819e5`
has **10,968 files / 5,783,889 raw bytes**. Cache bounds are 32 MiB raw data,
16,384 entries, 24 directory levels and 240-byte relative paths. Metadata/container
overhead is additional. Gameplay reads assets from this immutable cache; saves still
use the SD card. Restart after changing files.

The scanner indexes native models and texture files. Only registered gameplay
models and their dependencies are parsed/decoded for rendering. All other bytes
remain cached. This avoids attempting to fit thousands of unused sprites in 3DS
VRAM. Missing required files, invalid JSON, unresolved parents/texture variables,
corrupt PNGs and unsupported required source types produce the bottom-screen error
prompt. No partially loaded catalog enters gameplay.

## Models, items and atlases

`Content.hpp` provides stable gameplay IDs. Model inheritance and texture variables
are resolved from actual JSONs; namespace `minecraft:` is accepted. Paths with
traversal, absolute paths and unsupported namespaces are rejected. Parent chains
are limited to 16, texture-variable chains to 32, cuboids to 16 per model.
Axis-aligned cuboids use native `from`, `to`, faces, UVs, 90-degree face rotations
and matching `cullface`. Collision/targeting remain full-voxel. Element rotation,
blockstate variants, custom gameplay behavior from JSON and arbitrary resource-pack
compatibility are not implemented.

Native item definitions under `items/` select models under `models/item/`; idle
`condition.on_false` and `select.fallback` branches supply normal icons. These are
branches in actual repository JSON, not fallback assets. Required unsupported
item renderers fail explicitly. Builtin generated model roots are recognized.

Actual `blocks`, `items`, `gui` and `chests` atlas definitions supply `directory`
and `single` sprite mappings. Unused armor-trim `paletted_permutations` entries are
shape-checked but not synthesized: no registered item requests them. Other unused
atlas files remain in RAM. A required unsupported source is an error.

Native special cases have explicit renderers:

- Lava's particle-only JSON supplies its PNG; the actual disk `block/cube_all`
  template supplies its solid hazard geometry.
- Chest's native entity model uses a static cuboid and the actual chest skin.
  There is no lid animation. Shield/chest inventory icons show their source skins,
  not a rendered 3D item preview.
- Villagers use cuboids with UVs into the actual native villager skin. Species and
  biome/profession overlay rendering are not part of this pass.

The first animation frame indicated by `.png.mcmeta` is extracted; animation is
not advanced. Grass and oak foliage use the repository colormap; birch uses its
fixed species tint. Full biome interpolation, translucent sorting and light
propagation are future work.

## Atlas pixels and GPU state

The current registered dependencies produce **232 sprites in a 512×512 atlas**:
The atlas consumes 1 MiB of RGBA8 VRAM. A bounded skyline packer fills the gaps
beside and below the GUI panels; row packing required 4 MiB for the same pixels. Every sprite copies actual decoded PNG pixels. Native GUI
sheets retain their used 176-pixel panel width while unused transparent margins
are cropped. Packing uses extruded one-pixel gutters, half-texel UV insets, and
nearest filtering. POT sizes grow from 256 to 1024; overflow fails visibly.

`tileRgba8` vertically flips RGBA pixels into PICA 8×8 Morton tiles and ABGR byte
order. `GpuAssets` flushes a linear staging allocation, uploads through
`C3D_TexUpload` into `C3D_TexInitVRAM`, then copies VRAM back into staging and
compares bytes. That hardware check executes at startup; host tests verify the
pixel conversion but cannot execute the PICA transfer.

Opaque and cutout faces have separate mesh ranges. Native grass overlays must
be alpha-tested individually: treating every grass face as opaque paints transparent
black pixels over the base face. Equal-depth acceptance permits coplanar overlays.
World state binds the shared atlas explicitly, and UI setup disables the world
alpha test before Citro2D blending. Native HUD/inventory/chest images share the same
texture through stable Citro2D subtexture views.

## Checks

`make pipeline-test` uses the real repository tree, compares packed source pixels,
checks tint/alpha layers and atlas dimensions, rejects a renamed `blocks` folder,
missing HUD PNG and missing texture references, and loads successfully after its
temporary source tree has been deleted (RAM-only access). It also meshes a complete
25-column village window and verifies save migration/chest persistence. It does
not emulate the graphics processor. Device checks must confirm upload verification,
UV orientation, cutouts, UI blending and bottom-screen visibility.

## Expanded registry

The registry now has 105 non-air blocks and 191 items: additional stone/wood/ore
families, all wool and terracotta colors, storage blocks, purpur/End stone bricks,
furnace/crafting table, tool tiers, armor and food. All model and icon dependencies
resolve from the supplied repository. Registration does not implement every
behavior associated with an item: hoes currently have tool/combat traits, while
farming is still pending; chorus fruit currently supplies food without teleportation.
Multilayer leather icons currently use their base layer; dye/overlay rendering and
full equipment rendering remain pending. GUI armor and furnace sprites are native.

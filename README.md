# 3DS Craft — New Nintendo 3DS voxel template

**Target: the complete game through Java 1.9. Full parity is not implemented yet.**
This build expands the working foundation to 105 blocks, 191 items, 81 recipes,
tiered tools, armor, ore veins and fuel-driven furnaces. See the explicit
[remaining systems](docs/survival-1.9.md#complete-game-target-and-remaining-work).
No additional production files were introduced: 20 `.cpp` files and 23 headers.

A C++17 / devkitARM engine base with a **single 320 × 240 bottom-screen view**,
Citro3D world/animals and Citro2D HUD/error labels. There is no top-screen render
target or top-screen draw submission. **Circle Pad moves; C-Stick looks** (positive Y looks up by default).

Features include seeded terrain with caves/trees/iron ore, RAM-cached assets and a
runtime power-of-two atlas, Survival/Creative worlds, Creative-only Superflat,
Overworld/Nether/End travel, crafting, health/hunger/death/respawn, off-hand shields,
attack cooldowns and Elytra gliding. Sheep, pigs, cows and a hostile chase script
use colored box models. This is a playable homebrew foundation with simplified
rules; [Survival and 1.9 scope](docs/survival-1.9.md) describes the implemented loop
and its limits.

Terrain startup synchronously primes the nearest 3×3 columns, flushes VBOs,
uses separate opaque/cutout passes and reserves a larger libctru main stack.
The HUD reports drawn/active chunks and vertices. SELECT → Terrain toggles a
solid diagnostic view. These changes address identified pipeline weaknesses;
**visual verification on the affected handheld is still needed**.

## Build

Install devkitPro's **3ds-dev** group and **3ds-libpng** package using its package manager:

```sh
sudo dkp-pacman -S 3ds-dev 3ds-libpng
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=$DEVKITPRO/devkitARM
export PATH=$DEVKITPRO/tools/bin:$DEVKITARM/bin:$PATH
make -j2
```

The Makefile uses the official devkitARM `3ds_rules` build system. CMake is not
required. It enables C++ exceptions, assembles the PICA vertex shader with Picasso,
links Citro2D/Citro3D/libpng/zlib/libctru, and reads assets/saves from the SD card.
There is no RomFS dependency. Relevant flags are `-std=gnu++17 -fexceptions`,
`-Ithird_party`, and `-lcitro2d -lcitro3d -lpng -lz -lctru -lm`; JSON is header-only.
`make deps` downloads the **nlohmann/json 3.12.0** single header with a pinned SHA-256;
subsequent builds use the verified local copy. The first build needs curl, sha256sum,
and network access. The upstream header retains its MIT license notice.

Copy `3ds-craft.3dsx` to `sdmc:/3ds/3ds-craft/3ds-craft.3dsx`, then launch it through
a homebrew loader on a **New 3DS, New 3DS XL, or New 2DS XL**. Old models receive an
unsupported-hardware message. **Also copy the repository’s `assets/` to the SD root as
`/assets/`**. Both the 3DSX and CIA require that directory. Saves are created under
`sdmc:/3ds/3ds-craft/saves/` automatically. No save directories are included in the
download, so installing the asset tree does not overwrite worlds.

### Optional CIA with explicit New 3DS resources

Install [makerom 0.18.4 or newer](https://github.com/3DSGuy/Project_CTR/releases), then:

```sh
make cia
```

`packaging/new3ds.rsf` requests:

| Setting | Request |
| --- | --- |
| `SystemModeExt: 124MB` | 124 MiB application memory on retail New 3DS |
| `CpuSpeed: 804MHz` | New 3DS CPU speed |
| `EnableL2Cache: true` | L2 cache |
| `CanAccessCore2: true` | Permission to create a worker on core 2 |
| `IdealProcessor: 0` / `AffinityMask: 1` | Main application thread on core 0 |

The executable also calls `osSetSpeedupEnable(true)`. A `.3dsx` has **no exheader**:
its loader decides the accessible RAM and core permissions. It cannot independently
request 124 MiB. libctru's default heap allocator uses the process resource limit,
including extra RAM when granted; we do not hardcode a heap reservation that could
abort before `main()`. The usual libctru split leaves up to 32 MiB of linear RAM
and assigns the rest to the application heap. `__stacksize__` explicitly reserves
256 KiB for main; the terrain worker has a 128 KiB stack. The CIA exheader stack
setting alone does not override libctru's relocated main stack.

The worker tries `threadCreate(..., core_id=2, ...)`. If permission or thread
allocation is unavailable, it generates one column at a time on the main thread;
the bottom screen reports the active mode. Core 1 is reserved for system work here,
and core 3 is not available to normal applications. **178 MiB is a development-unit
mode, not the retail target.** The CIA is an unsigned homebrew test package requiring
a compatible CFW installation. Its `0xFCE01` unique ID is a development placeholder;
choose an ID appropriate for your project before distributing it. No Nintendo banner
or logo asset is included.

Platform references: [libctru thread permissions](https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/thread.h),
[libctru heap allocation](https://github.com/devkitPro/libctru/blob/master/libctru/source/system/allocateHeaps.c),
[makerom exheader settings](https://github.com/3DSGuy/Project_CTR/blob/master/makerom/src/exheader.c).

## Controls and bottom-screen HUD

| Input | Action |
| --- | --- |
| **Circle Pad / C-Pad** | Move relative to camera yaw |
| **C-Stick / C-Nipple** | Look; upward input looks upward |
| R | Hold to mine; press to attack with cooldown-scaled damage |
| L | Place/use/eat; hold to guard with an equipped off-hand shield |
| B | Jump; toggle equipped Elytra while airborne; respawn after death |
| A (hold) | Crouch; release to stand when there is headroom |
| ZL / ZR | Cycle the 12-entry building/tool/food hotbar |
| X / Y | Open or close inventory |
| Inventory D-pad + L | Move cursor / select any item |
| Inventory ZL / ZR | Previous / next page of 27 item types |
| L on furnace | Open native furnace panel; D-pad actions, L confirm, R close |
| SELECT | Crafting, equipment, dimension travel, terrain diagnostic |
| START in gameplay | Save and return to main menu |
| START in menus | Resume an active world, or exit from the main menus |
| Menu D-pad + L / touch | Navigate and select |
| Menu R | Return / cancel editing |
| Error D-pad up/down | Scroll details; L/R returns from recoverable prompts |

Analog calibration applies a dead zone independently to each axis and
normalizes values to the libctru input range. Settings lets you change look speed,
dead zone and inverted Y **for the current session**. Crouching reduces body height
from 1.8 to 1.3 blocks and movement speed; it does not implement ledge protection.
Inventory stops player movement/look/actions, while gravity, animals and time continue.

## Menus and SD worlds

`App` owns the main menu, world selection, creation, gameplay, settings, actions
and crafting states. Creation offers **native `swkbd` naming** (32 characters),
a three-word English name generator, seed, Survival/Creative, and world type.
Superflat is disabled in Survival. A custom bottom-screen keyboard remains
available for names/seeds. Nintendo's software-keyboard applet controls its own
screens and may use the top screen; choose **Bottom keyboard** if needed.
World names are display strings, never directory paths.

```text
sdmc:/
  assets/minecraft/               Copy repository assets/minecraft here
    textures/block/*.png          Individual sprites (nested folders supported)
    textures/item/*.png
    models/block/*.json           Models and parent templates
    models/item/*.json
    atlases/*.json                Sprite dependency/alias sources
  3ds/3ds-craft/
    3ds-craft.3dsx                 For Homebrew Launcher
    saves/world-<generated-id>/
      world.0.json
      world.1.json
```

Save format 5 stores name/seed/generator, mode/type/dimension, player transform,
inventory/selection, day time, health/hunger/equipment wear, dropped inventory and
three dimension edit journals/arrival points chest/furnace contents, equipped armor and tool wear. START from gameplay saves
before leaving; a failed save keeps the session open and shows an error. Normal
system termination also attempts to save. There is **no periodic autosave**.
Animals are spawned anew on each load; animal health/positions and settings are
not serialized.

Writes validate the entire state, write/flush/fsync a temporary file, then commit
into the inactive generation slot. Loading chooses the highest fully validated
committed generation and ignores `.tmp` files. If one slot is corrupt, the other
is recovered and marked on screen; two corrupt slots produce a descriptive error.
This preserves the other file during a failed replacement, but cannot guarantee
recovery from filesystem-wide SD/FAT damage or power loss. Back up valuable saves.
The directory list is bounded to 64 worlds; names are display text, never paths.

Format-1 through format-4 saves migrate to format 5 while preserving their generator
and stable content IDs. Legacy worlds use Creative rules to preserve earlier
unlimited-health behavior. New worlds use **generator 5**, adding ore veins to the existing template villages and terrain.
The default seed is **42**, with a village centered at (24, 31). Existing saves keep
their previous generator and terrain. All versions remain deterministic.

The crosshair is centered at **(160, 120)**. FPS is at the **top left**, sampled over
half-second windows using actual elapsed frame time rather than the capped physics
delta. Dynamic **RAM usage is at the bottom left**, refreshed every half second.

`RAM live` means newlib `mallinfo().uordblks` plus used libctru linear-heap bytes;
the denominator is the reserved application + linear heap capacity. It reflects
allocations and frees rather than the process's already-reserved heap commitment.
It excludes VRAM, executable code and allocations outside those allocators. Debug
counts show drawn/active sub-chunks, worker mode, and total vertices. Inventory and
action feedback share a translucent strip at the bottom of the world view.

## Project structure

```text
include/             Public .hpp interfaces
source/main.cpp      Platform setup and top-level error boundary
source/App.cpp       State machine, exact control mapping, session/save lifecycle
source/Menu.cpp      Portable bottom-screen name/seed editor model
source/WorldStore.cpp Validated two-generation SD world persistence
source/FileSystem.cpp Bounded traversal and checked directory/file operations
source/AssetArchive.cpp Every asset file and directory cached before gameplay
source/AssetCatalog.cpp Runtime atlas packing, inherited JSON models and PICA swizzle
source/GpuAssets.cpp VRAM C3D_Tex ownership/upload and Citro2D inventory icons
source/Assets.cpp    Checked libpng and nlohmann/json file decoders
source/Core.cpp      Versioned noise/caves/trees, reference mesher, camera, collision
source/Village.cpp   Deterministic house/path/well templates across chunk boundaries
source/ModelMesh.cpp Atlas-safe cuboid faces, normals, UVs and boundary culling
include/Content.hpp  Stable block/item registry, release tags and mob definitions
source/Crafting.cpp Shaped/shapeless matching and atomic inventory transactions
source/BlockStore.cpp Implicit terrain, bounded edit journal, edit-driven dirty keys
source/World.cpp     Worker snapshots, sparse GPU meshes, horizontal streaming
source/Survival.cpp  Vitals, timed mining/combat, off-hand, gliding, arrivals/names
source/Gameplay.cpp  DDA rays, placement/breaking, inventory, day clock, FPS sampling
source/Animals.cpp   GeneralAnimal components and Sheep/Pig/Cow scripts and models
source/Metrics.cpp   Platform allocator usage sampling
source/Renderer.cpp  Bottom-only world, menus, inventory, progress/error labels
shaders/             PICA200 shader with sunlight and texture coordinates
assets/              Unmodified upstream repository PNG/JSON and other assets
packaging/           Optional New 3DS CIA metadata
scripts/             Pinned dependency download
tests/               Portable logic/filesystem tests and World.cpp integration tests
docs/                Asset format details and researched Release 1.0–1.8 scope
content/             Machine-readable era planning catalog
```

### Sparse world, editing, and memory lifetime

The world is partitioned into **16³ sub-chunks**, with four vertical levels within
a 64-block world. A 5 × 5 horizontal column window bounds streaming; the 32-block
far plane fits inside that footprint once streaming catches up.

**There are no persistent block arrays or empty sub-chunk slots.** Authoritative
block queries combine immutable procedural terrain with a bounded map of player
edits. Meshes have a one-voxel halo from the same source, so adjacent opaque solids
hide their internal faces across both horizontal and vertical boundaries.

The lifecycle is:

1. Generation samples a temporary 18×18 column halo, with 64 block values per
   sampled column. It builds each vertical layer once to discover cave surfaces.
   These temporary arrays are destroyed after the job; they are not chunk storage.
2. Zero-vertex results are discarded immediately. They have **no active-map entry,
   block array, GPU buffer, update call, or per-frame render/culling visit**.
3. Loaded horizontal column records prevent repeatedly requesting empty results;
   there is no per-empty-sub-chunk tombstone or polling list.
4. Placing a block in an absent region triggers its reconstruction and neighbor
   remeshing as needed. Removing its last visible geometry deletes its entry and VBO.
5. **Enclosed solid chunks also have zero vertices. Digging that exposes them must
   rebuild them**, so the newly exposed terrain remains visible. Entirely air chunks
   remain absent until placement supplies geometry. This is an edit event, not a
   background scan of absent chunks.

A small block-edit journal remains independent of chunk residency. Removing a
procedural block stores an air override; restoring its original material removes
the override. Edits survive streaming out and back and are serialized with the world on save. The limit is **8192 modified block coordinates**;
new edits at the limit are rejected with on-screen feedback, without consuming
inventory. Restoring original blocks frees journal entries.

The worker receives a snapshot of edits in the requested column and its halo.
Results carry a revision; results from before a player edit are discarded instead
of overwriting newer state. Only the main thread queries/mutates the live edit store,
allocates GPU memory, or submits graphics commands. A 64-entry, 64-byte-per-column procedural cache
speeds up collision queries without creating chunk objects. Player edits override
that immutable cache immediately.

Visible meshes alone occupy the active map. CPU generation vectors are destroyed
after upload. VBOs use `linearAlloc`, have a **12 MiB live budget**, and are replaced
only after `C3D_FrameBegin` waits for prior GPU work. A replacement is allocated
before the old buffer is freed, so transient allocation failure is reported cleanly.
Vertices are eleven floats (44 bytes): position, RGB, normal and UV.
The atlas model mesher emits per-block cuboid faces with resolved UVs, avoiding
texture bleeding or stretching over multiple blocks. Each visible sub-chunk needs
up to two world-atlas draw calls (opaque/cutout). This avoids CPU
mesh rebuilds as the sun moves. `C3D_FrameEnd(0)` keeps the standard linear-heap
flush needed for Citro2D's internal buffers.

Frustum culling visits only active sub-chunks. GPU backface culling and depth testing
handle remaining visibility; there is no whole-chunk occlusion query. Distance
management remains the alternative to multiresolution LOD. The nearest 3×3 columns are generated synchronously before entering gameplay;
outer columns stream through the worker. Collision reads authoritative blocks even before
meshes arrive; interactions wait until their target columns are loaded.

### Block interaction and inventory

A normalized voxel DDA ray traces up to **five blocks** from the camera. Breaking
adds the actual material to inventory. Placement targets the preceding empty voxel,
uses the selected stack, and rejects overlaps with the player or an animal, occupied
cells, and world limits. Shared collision immediately observes all edits.

Survival starts empty; Creative starts with 64 of every item and places blocks
without consumption. There are 191 stable item IDs and 105 solid/hazard block IDs.
The aggregate inventory limit remains 999 per type. All items can be selected;
non-block items cannot enter the voxel-placement path. The native inventory panel displays items in a nine-column grid, with eight pages
selected by ZL/ZR. Both chest panes use the selected page. Failed edits/crafts do not consume ingredients. Equipment has a
separate off-hand slot and Elytra slot; SELECT opens crafting/equipment/travel, including equip/remove selected armor.
Place a furnace and press L to deposit input/fuel or collect output; fuel burns over
time and one item cooks in ten seconds. Loaded furnace records tick in gameplay
and menus, with a 60-second catch-up cap. Inactive dimensions and unloaded columns
pause their furnaces.
See [the Survival guide](docs/survival-1.9.md) for the complete playable loop.

### Day and night

`DayNight` wraps every **1200 real seconds**, equivalent to 24,000 ticks at 20 ticks
per second, and starts at noon. Smooth daylight/twilight changes both the background
clear color and ambient/directional lighting. Sun direction moves across the sky.
The PICA vertex shader evaluates `ambient + diffuse * max(dot(normal, sun), 0)` for
both terrain and animal models; terrain texture color modulates the result. Night retains a small ambient floor for visibility.
Terrain also applies classic per-face brightness (up 1, down 0.5, east/west 0.6,
north/south 0.8). There is no shadow mapping, skylight propagation or per-block light storage.
The clock uses uncapped elapsed time, so slow rendering does not stretch the cycle;
resuming after suspension advances by the elapsed time.

### Component-based animals

`GeneralAnimal` owns small `PhysicsBody`, `TransformComponent`, `HealthComponent`,
and `WanderComponent` values. Shared logic supplies gravity, terrain collision,
health/damage, idle/wander state transitions, one-block obstacle jumping, blocked-path
turning, and cliff probes. There is no heavyweight ECS or per-frame script allocation.

| Script | Placeholder model | Trait | Drop |
| --- | --- | --- | --- |
| `SheepAnimal` | White body, brown face/legs | Longer grazing pauses; 8 health | 2 wool |
| `PigAnimal` | Pink body and snout | Faster wandering; 10 health | 2 pork |
| `CowAnimal` | Brown body with white patches | Slower movement; 12 health | 2 leather |

Three animals spawn near the starting point. The system is budgeted for up to 12;
AI runs within 24 blocks of the player and pauses outside that range. Animal faces
are assembled into one bounded draw buffer and use the same daylight shader. R
attacks within four blocks, scales damage with held item/cooldown, and respects intervening terrain.
Defeated animals are removed and their species drop is added once to inventory.
AI uses local steering rather than global pathfinding; animal-to-animal collision,
persistent entities remain an extension. In Survival, a bounded hostile zombie
script spawns at night/in other dimensions, chases and causes contact damage.

The shared player/animal physics uses axis-separated AABBs, gravity and substeps of
at most 0.15 blocks. Player bounds are 0.6 × 1.8 blocks; animal bounds conservatively
cover their box models. Physics deltas are capped at 50 ms. A ±8192 block horizontal
boundary limits float precision loss.

### Atlas, models and era content

Startup caches **every file and directory** under `sdmc:/assets/`: the current
repository contains 10,968 files totaling 5,783,889 raw bytes. Models are read from
**`minecraft/models/block/` and `minecraft/models/item/`**, preserving native resource
names. Actual atlas definitions and modern `minecraft/items/` definitions resolve
PNG dependencies. No replacement texture or embedded model JSON is supplied for
missing files: errors identify the offending resource.

All raw assets remain in RAM. The 232 sprites required by the implemented blocks,
items, GUI and villagers fit one **512×512 RGBA8 atlas (1 MiB VRAM)**. Upload uses
PICA tiled ABGR pixels, a flushed linear staging buffer and `C3D_TexUpload`, followed
by a startup VRAM readback comparison. Grass's native transparent overlay receives
its own alpha-tested pass with equal-depth acceptance; colormaps tint grass/leaves.
This fixes identified upload/overlay risks; final visibility still needs a handheld.

The HUD uses repository hotbar, selection, heart, hunger and crosshair PNGs. Inventory
and chest panels use the native container sheets at one GUI pixel per screen pixel.
L on a chest opens its contents; D-pad selects, L moves one, R moves a stack, X/Y closes.
Chest storage is bounded to 64 containers per dimension and 999 of each item type;
nonempty chests cannot be mined. Storage remains an aggregate item inventory, not
vanilla per-slot stacks.

Normal generator-4/5 Overworlds can contain four-house villages with paths, chests,
a decorative well and three textured wandering villagers. Templates use global
coordinates, so structures remain continuous across sub-chunks. Existing worlds,
Superflat and other dimensions keep their generation rules. Villagers have shared
physics/health/wandering, without trading or persistent entity state.

See [the exact asset contract](docs/assets.md), [visual references](docs/visual-reference.md),
and the historical [1.0–1.8](docs/era-1.0-1.8.md) / [1.9](docs/survival-1.9.md) scope.
The repository pack contains modern asset conventions; gameplay scope and asset
format version are separate. Unsupported content remains cached, not instantiated.

## Validation

On a host with g++, libpng development headers, and ASan/UBSan runtimes:

```sh
make host-test
# Or run a focused check:
make core-test
make gameplay-test
make world-test
make assets-test
make pipeline-test
```

- Core: noise, greedy mesh surface area/winding/normals, sparse generated surfaces,
  negative coordinates, frustum, caves/tree determinism and boundary meshes, grounding/jumping/falling/walls.
- Gameplay: authoritative edits and limits, ray reach/adjacency, inventory
  transactions, collision after edits, placement overlap rejection, cycle timing,
  FPS sampling, crouch clearance/speed, animal AI/attack occlusion, hotbar mapping,
  crafting shape matching, capacity checks and atomic consumption.
- World: real `World.cpp` with small host allocation/thread/frame adapters. Tests
  cover sparse eviction/reactivation, stable idle allocation counts, horizontal and
  vertical mesh seams, exposed solids, edit persistence across streaming, replacement
  failure cleanup, and stale worker-result rejection using real host threads.
  A generator-2 world with 25 atlas-meshed columns also checks the 12 MiB VBO
  budget. These adapters **do not emulate PICA200 or libctru hardware behavior**.
- Assets: valid/malformed/missing PNG and JSON, image/input/depth limits.
- Pipeline: recursive discovery/bounds, metadata mapping/missing keys/corruption,
  NPOT atlas packing/gutters/non-overlap, inherited/nested models, geometry/UVs,
  pixel swizzle goldens, save roundtrip/generation/recovery/migration, invalid
  counts/coordinates/duplicates, failed file replacement, editor and analog calibration.

ARM compilation, shader assembly, 3DSX and CIA packaging were checked with devkitARM
GCC 16.1.0, libctru 2.7.0, Citro3D 1.7.1, Citro2D 1.7.0, 3ds-libpng 1.6.53, and
makerom 0.18.4. The earlier base engine was reported working by the user; this updated
terrain pipeline, native keyboard, dimensions and Survival integration need device
validation. Host tests exercise cached-only loading, atlas sources, dimension
arrivals/meshes/void, Survival rules, crafting, equipment, gliding and save migration.

On a New 3DS, check:

1. All graphics, HUD, help and errors appear on the bottom screen; test both launch paths.
2. Create with native/custom/random name, mode/type/seed; save/reload all three dimensions.
3. Circle Pad movement, C-Stick look, R/L actions, B jump, A crouch, ZL/ZR and X/Y bag.
4. Empty-layer placement/removal; trees across boundaries, underground caves, and saved edits.
5. Nonzero startup mesh counts, solid diagnostic mode, textures/cutouts, depth alongside animals.
6. Craft a tool/shield, equip off-hand, mine, eat, take fall/hostile damage, die/recover items, glide.
7. FPS response and live RAM changes during streaming/editing; worker mode per loader.
8. HOME/suspend/resume and START. On a copied test save, corrupt the newest generation
   to check fallback. Test missing/corrupt PNG/JSON and long-path error prompts.

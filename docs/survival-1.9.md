# Survival and Combat Update foundation

## Playable loop

1. Create a **Survival / Normal** world. The inventory starts empty. Circle Pad
   moves, C-Stick looks, B jumps, A crouches. Hold R to mine; press R to attack.
2. Collect wood, SELECT → Craft items, make planks/sticks and a wooden pickaxe.
   Use X/Y and ZL/ZR inventory pages to select it. Mine cobblestone, craft a stone
   pickaxe and furnace, then mine iron. Place the furnace, select iron ore and press
   L on it to deposit input. Return with coal, charcoal, wood or sticks as fuel.
   Collect ingots and craft iron tools, a sword, armor or a shield. Transactions
   reject unavailable ingredients or full destinations without item loss.
3. Pigs supply pork. Select pork and press L when hungry. Movement/attacks and
   regeneration spend food; exhaustion first consumes saturation. Health slowly
   regenerates with enough food. Empty hunger causes starvation damage.
4. Night/in other dimensions can spawn a hostile chase script. Fall, lava contact,
   void and melee damage affect health. Native heart/hunger sprites appear above the hotbar.
5. After death, B respawns in the Overworld. Inventory/equipment are dropped into
   one recoverable bag at the death coordinate/dimension. SELECT lists its location;
   entering a three-block radius picks up items with available capacity. A later
   death replaces that bag. Items dropped in the void may be unrecoverable.
6. SELECT → Travel switches among the Overworld, Nether and End. Terrain and edits
   are dimension-specific; arrival searches for a supported, unobstructed position.
   Nether first-entry coordinates use 1:8 scaling. Returning prefers recorded arrival.
7. First End arrival awards an Elytra to exercise the feature. Select it, then
   SELECT → Equip Elytra. B while airborne toggles gliding. Look down to dive,
   up to trade speed for lift. Collisions can hurt. Durability starts at 432,
   drains once per gliding second and stops flight at 1. Repair uses two leather.

**Creative** ignores damage/hunger, starts with every item, mines quickly, and
places without consumption. Only Creative exposes Superflat (stone, two dirt
layers, grass). Dimension terrain remains Nether/End-specific even for Superflat.

## Off-hand and combat

Select an item in the bag, then SELECT → Equip/remove off-hand. The off-hand holds
one item outside the inventory. Selecting the same type and repeating the action
returns it. Failed exchanges leave both inventories unchanged.

Main-hand block placement/food has use priority. Otherwise L tries off-hand food,
blocks, or a shield. Hold L to guard with a shield; it takes 0.25 seconds to raise,
slows movement and protects against frontal blockable melee. Rear, fall, starvation,
void and lava damage bypass it. Blocking wears/breaks the shield. Only the main
hand attacks. The HUD shows current attack charge and equipped off-hand item.

Sword attack speed is 1.6/s, pickaxe 1.2/s, unarmed 4/s. Damage scales quadratically
with charge; rapid clicking deals reduced damage. Switching selected items resets
charge. Elytra gliding uses an inexpensive lift/dive/drag model with substepped
collision; there are no post-1.9 firework boosts.

## Persistence and bounded implementation

Format 5 persists mode/type/dimension, vitals, off-hand/Elytra and their wear,
dropped inventory, three independent 8192-edit journals, chest/furnace contents, equipped armor and tool wear. Legacy formats 1/2
retain their generator and migrate into Creative rules. Raw assets remain in RAM
across world/dimension transitions. Format 3 retains its mode and generator;
its old empty off-hand sentinel migrates explicitly. New worlds use generator 5
with villages and additional ore veins; previous generator versions stay unchanged. Only the active dimension owns terrain VBOs;
old GPU meshes are drained/released before destination allocation. If preparation
fails, the previous world state remains available and its meshes rebuild on resume.

This delivers the listed Survival systems in a small playable engine. It is **not
full vanilla Minecraft 1.9 parity**. Current simplifications are explicit:

- Travel uses the action menu; there are no constructed portal blocks or End boss.
- The End is one finite island, Nether a noise cavern. Lava is a solid contact hazard,
  without fluid flow. There are no generated fortresses, End cities or loot chests.
- Recipes use a list UI with 81 registered recipes, including shaped tool/armor rules.
  Crafting-table interaction opens that list; grid UI and table-distance gating remain
  pending. Furnaces use persistent input/fuel/output slots, ten-second cooking, fuel
  consumption and loaded-column ticking. They do not yet award XP or animate the
  lit block texture. Fuel coverage is a subset of vanilla burnable items.
- Melee damage, hunger/regen and shield balance are simplified. Frontal shield blocks
  currently absorb all damage, unlike historical 1.9 melee reduction. Shield durability
  is 336; historical off-by-one durability behavior is not reproduced.
- Tools track wear for the active item of each type; armor tracks equipped-slot wear
  and preserves it through equip/remove. Aggregate stacks/containers do not preserve
  per-instance NBT or individual durability. Normal attacks, falling critical hits and
  grounded sword sweeps are integrated, with bounded target scans and transactional
  drops. Armor/toughness reduces blockable melee damage; full damage-type rules,
  fractional entity health, sprint knockback, enchantments, XP, projectiles, axe
  shield-disable and status effects remain pending.
- Elytra retains the player's standing AABB and uses approximate gliding physics.
  Equipment grant/repair are template progression hooks, not End-city/anvil simulation.
- Animals/hostiles regenerate on world load/travel; positions/AI are not saved. The
  hostile script is a colored box with local steering, no navigation graph.
- Other 1.9 additions (shulkers, chorus growth/teleportation, tipped arrows, lingering potions,
  End gateways and dragon respawning) are listed as planned in the content catalog.

## References

Historical feature scope: [Java Edition 1.9](https://minecraft.wiki/w/Java_Edition_1.9).
Historical/current differences checked in the [shield history](https://minecraft.wiki/w/Shield#History)
and [Elytra history](https://minecraft.wiki/w/Elytra#History): shields/dual wielding/
attack timing and Elytra arrived in 1.9, firework propulsion came in 1.11.1, and
leather repair preceded phantom membranes. Modern atlas source format:
[atlas resource files](https://minecraft.wiki/w/Atlas).

Platform APIs: [libctru software keyboard](https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/applets/swkbd.h),
[libctru relocated main stack](https://github.com/devkitPro/libctru/blob/master/libctru/source/system/stack_adjust.s),
[Citro3D buffer layout](https://github.com/devkitPro/citro3d/blob/master/include/c3d/buffers.h).
Native keyboard is a system applet and controls its own displays. All application
rendering still targets GFX_BOTTOM; the custom bottom keyboard remains available.

## Terrain diagnosis on hardware

On entering gameplay, nearby geometry should already have nonzero vertex counts.
Opaque faces bypass texture alpha discard; leaves and native grass overlays have a
separate alpha-tested pass. Equal-depth testing preserves coplanar grass overlays.
The atlas upload is read back from VRAM and compared before gameplay; a mismatch
raises a descriptive error. No texture fallback is generated.
The renderer restores shader/attributes/texture/depth/blend/stencil each frame and
VBO writes are explicitly cache-flushed. SELECT → Terrain: solid diagnostic also
bypasses frustum/backface culling. If solid terrain appears but textured terrain
does not, inspect atlas upload/UVs; if drawn/active is zero only in normal mode,
inspect camera/culling; if vertices stay zero in the Overworld, capture the exact
loading error. Empty space away from the End island legitimately has zero meshes.

Host tests cannot demonstrate PICA200 visibility, native-applet behavior or core-2
scheduling on the affected device. Report the drawn/active/vertices HUD values and
whether the solid diagnostic changes visibility when testing the new build.

## Villages and containers

Generator 4 adds seeded Plains villages containing four houses, connecting paths,
a decorative dry well, empty chests and three villagers wearing the repository skin.
Default seed 42 has a village centered at X=24, Z=31. There is no trading, profession
overlay, villager persistence or generated chest loot. L opens a chest, D-pad selects
a type, L transfers one and R transfers the available stack. X/Y closes it. Chests
persist up to 64 records per dimension; empty them before mining. Inventories retain
aggregate counts rather than vanilla stack slots.

## Complete game target and remaining work

The user target is the **complete game through Java 1.9**, including the earlier
releases. This build is still an incomplete engine implementation. The provided
asset pack does not supply gameplay code. A registry entry, texture or recipe
is not evidence that the associated whole system is implemented.

| Area | Current state / remaining work |
| --- | --- |
| Content | 105 blocks, 191 items; remaining block/item/state families pending |
| Inventory | Eight pages of aggregate counts; vanilla slots, stack limits, item NBT, containers preserving per-instance wear pending |
| Tools/armor | Five material tool sets; five armor sets with mitigation/wear; enchanting/anvils/repair/XP and complete equipment rendering pending |
| Crafting/furnaces | 81 recipes, shaped/shapeless matcher, persistent timed smelting; remaining recipes, grid UI, hopper automation and XP pending |
| World | Bounded 64-high noise terrain, caves, trees, villages, ore veins, three dimensions; all biomes, generated structures and vanilla terrain parity pending |
| Block simulation | Placement/mining; fluids, gravity blocks, fire, weather, light propagation, redstone, pistons, rails and explosions pending |
| Farming | Crops, growth ticks, irrigation, breeding, fishing and food-specific effects pending |
| Entities | Sheep/pig/cow/villager and basic hostile script; remaining mob roster, navigation, spawning/despawning, equipment, trading and persistence pending |
| Combat | Cooldowns, critical/sweep, off-hand/shields, armor, gliding; projectiles, effects, brewing, enchantments, historical shield balance and complete damage rules pending |
| End/progression | Menu travel and finite island; portals, strongholds, dragon/respawn, outer islands, cities/ships, shulkers, gateways, chorus growth pending |
| Presentation | Bottom-screen textured blocks/HUD/containers; full item/hand/entity models, particles, audio and animations pending |
| Modes/interfaces | Survival/Creative template; Adventure/Spectator, difficulty, achievements, commands, resource-pack settings and complete accessibility/localization pending |
| Multiplayer | No network protocol, server, replication or multiplayer UI implemented |

No new production files were added in this expansion. Content traits remain in
`include/Content.hpp`, portable mechanics in existing Gameplay/Survival/Crafting/Animals
files, integration in App/Renderer, and persistence in WorldStore. Hot paths retain
bounded loaded-column meshes, a 12 MiB VBO budget, a single shared atlas and at most
64 furnace records per dimension. These are allocation/algorithm limits, not measured
3DS frame-rate claims.

New worlds use generator 5. Format-5 saves append content IDs; formats 1–4 retain
their exact former array widths and empty-item sentinels during migration. Previous
generation algorithms are preserved. Furnaces save even while burning; only loaded
columns of the active dimension tick, and suspend catch-up is bounded to 60 seconds.

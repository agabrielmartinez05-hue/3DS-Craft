# Java Release 1.0–1.8 content scope

This target includes the content available in **Java Release 1.0 through 1.8**,
including the older blocks and animals still present in 1.0. It excludes Beta 1.8
and post-1.8 features such as shields, dual wielding, elytra and attack cooldowns.
The engine remains a small homebrew template, not a complete implementation of those
releases or their world/save formats.

## Researched feature map

Representative additions and the engine systems they require:

| Release | Blocks/items/entities and mechanics | Extension point |
| --- | --- | --- |
| [1.0.0](https://minecraft.wiki/w/Java_Edition_1.0.0) | Enchanting, brewing, breeding, the End and Ender Dragon | Recipe processors, status effects, dimensions and mob scripts |
| [1.1](https://minecraft.wiki/w/Java_Edition_1.1) | Spawn eggs, superflat worlds, sheep wool regrowth | Item actions, generator profiles, species components |
| [1.2.1](https://minecraft.wiki/w/Java_Edition_1.2.1) | Jungle trees, ocelots, iron golems, redstone lamps | Biome/structure palette, AI definitions, block state |
| [1.3.1](https://minecraft.wiki/w/Java_Edition_1.3.1) | Emeralds, trading, ender chests, tripwire, writable books | Offers, persistent containers, circuit events, item data |
| [1.4.2](https://minecraft.wiki/w/Java_Edition_1.4.2) | Beacons, anvils, carrots/potatoes, Wither, witches and bats | Effects, repair recipes, crop ticks, flying/boss scripts |
| [1.5](https://minecraft.wiki/w/Java_Edition_1.5) | Hoppers, comparators, droppers, quartz and daylight sensors | Scheduled circuits and transactional inventory transfer |
| [1.6.1](https://minecraft.wiki/w/Java_Edition_1.6.1) | Horses/donkeys/mules, leads, horse armor, carpets and resource packs | Mount/leash components, item metadata, model assets |
| [1.7.2](https://minecraft.wiki/w/Java_Edition_1.7.2) | Acacia/dark oak, stained glass, new biomes, fish and fishing changes | More tree profiles, translucent pass, biome registry, loot |
| [1.8](https://minecraft.wiki/w/Java_Edition_1.8) | Granite/diorite/andesite, slime blocks, prismarine, banners, rabbits, guardians, ocean monuments and armor stands | State/material variants, bounce physics, structure jobs, equipment/model components |

`content/era-1.0-1.8.json` is a machine-readable **planning catalog**, with references,
release tags and explicit status. It is representative rather than exhaustive and
is not loaded as enabled content. The linked release histories were retrieved and
checked during this update.

## What exists in code

- `Content.hpp`: append-only block/item IDs, item placement mapping, block render
  layers, drops/hardness metadata, `Release` tags and shared mob descriptors. `V1_0`
  means “available in the release baseline,” not necessarily introduced in 1.0.
- Playable blocks: grass, dirt, stone, oak/birch logs, oak/birch leaves and oak planks.
  Tree blocks can be broken, collected, selected, placed and saved. Leaves use
  cutout alpha. Drops are template rules, not full vanilla loot tables.
- `GeneralAnimal` components and three species scripts now share mob definitions.
  The existing demo health/speed/drop tuning is retained; it is not an assertion of
  historical Minecraft balance. More species can use the same component structure.
- `Crafting.hpp/.cpp`: bounded 2×2/3×3 grid matching, translated/mirrored shaped
  recipes, shapeless recipes, release filtering and an atomic inventory commit.
  Log-to-planks and vertical planks-to-sticks recipes are included. The API is tested;
  a crafting screen is not implemented. Birch currently produces the single planks
  type; separate wood variants can be appended to the registry.
- `RecipeKind::Smelting/Brewing` reserve processor categories; furnace fuel/time,
  brewing state and enchanting are not implemented.
- Models and item metadata are loaded independently of procedural generation. New
  render definitions do not automatically add gameplay IDs or behaviors.

## Adding content without breaking saves

1. Append IDs to `Block`/`Item`, extend `BlockTypes`/`ItemTypes`, add models and
   textures, and map the placeable item/drop. Do not reorder IDs: existing saves use
   them. Bump the save schema when changing inventory layout or adding block states.
2. Keep namespace strings and variant metadata in the content layer; do not equate
   the enum ordinal to a Java numeric block ID. A future block-state schema should
   carry orientation, growth/power level and wood/color variants explicitly.
3. Attach behavior to components (container, crop, circuit, mount, effect). Keep
   tickable block entities in a separate sparse event registry; never bring empty
   sub-chunks back into a per-frame processing loop.
4. Define recipes with a minimum `Release`. Keep matching separate from inventory
   mutation. Existing `craft` checks all costs and output capacity on a copy before
   committing. Add a crafting UI or processor only when its lifecycle is supported.
5. Keep translucent materials in a future sorted render pass. The current atlas
   supports opaque/cutout rendering; assigning a texture does not supply fluid or
   glass mechanics, biome tinting, connected textures, or redstone behavior.

## Primary model-format reference

The model loader was checked against the **original Mojang 1.8 client assets**,
including `block/cube`, `block/cube_all`, `block/cube_column`, `block/half_slab`,
`block/grass` and `item/stick`. The official [version manifest](https://piston-meta.mojang.com/mc/game/version_manifest_v2.json)
locates the 1.8 release metadata and client download. The inspected client SHA-1 is
`d722504db9de2b47f46cc592b8528446272ae648`. Those files were inspected in scratch space;
the running engine now uses the actual upstream repository assets exclusively.

The historical pack and current repository both use singular `models/block` and
`models/item`. References such as `minecraft:block/cube_all` resolve directly to
`assets/minecraft/models/block/cube_all.json`. The repository contains modern GUI,
atlas and item-definition conventions; historical gameplay scope does not imply
that its assets are an original 1.8 pack. See [supported assets](assets.md).

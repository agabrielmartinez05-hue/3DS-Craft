# Visual reference and bottom-screen adaptation

The official [Around the Block: Plains](https://www.minecraft.net/en-us/article/around-block--plains)
article describes Plains and its villages and provides a screenshot carousel,
including [the first Plains screenshot](https://www.minecraft.net/content/dam/minecraftnet/games/minecraft/screenshots/plains-carousel%20(1).jpg).
The official article text and image URLs were retrieved through a reader proxy;
direct image retrieval failed in this sandbox. No pixel-level comparison against
those official screenshots is claimed.

Actual repository inventory, chest, villager and packed atlas images were inspected.
The implementation uses those source pixels, including native nine-column slot
spacing, rather than drawing replacement UI artwork.

The 320×240 adaptation uses a 70-degree vertical perspective, 4:3 aspect ratio,
one world unit per block and a standing eye height of 1.62 blocks. The native
182×22 hotbar sits at (69,203); hearts and hunger sit above it. The 15×15 crosshair
is centered at (160,120). Inventory panels retain a 176-pixel width; a three-row
chest panel is composed from the native larger chest sheet. FPS and RAM labels
reserve the top and bottom edges.

Terrain uses grass/foliage colormaps, nearest texture filtering and face brightness
of up=1, down=0.5, east/west=0.6, north/south=0.8, modulated by the existing day/night
cycle with a small ambient floor. This approximates vanilla readability within the
3DS budget; there is no smooth ambient occlusion, skylight propagation or shadows.
Village templates are original compact structures using native block assets, not
copies of official world-generation data.

A physical New 3DS pass remains necessary for perspective/input feel, PICA texture
orientation and blending, display colors, structure visibility and frame rate.

#include "App.hpp"
#include "Error.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace voxel {
void App::loadAssets() {
    if (assets_)
        return;
    auto candidate = std::make_unique<GpuAssets>();
    candidate->load("sdmc:/assets", progress());
    assets_ = std::move(candidate);
}
Progress App::progress() {
    return [this, last = u64{0}](const std::string& text, unsigned done, unsigned total) mutable {
        const auto now = osGetTime();
        // Thousands of small files must not each wait for a display frame.
        if (last && now - last < 50 && !(total && done >= total))
            return;
        last = now;
        if (!aptMainLoop())
            throw Error("Loading interrupted by system exit");
        renderer_.loading(text, done, total);
    };
}
void App::beginWorld(const std::string& id) {
    // Construct everything off to the side. A bad save/asset never commits a
    // half-loaded gameplay state or discards an existing session.
    loadAssets();
    const auto save = saves_.load(id);
    session_ = save;
    renderer_.loading("Generating " + save.name, 0, 1);
    auto world = std::make_unique<World>(save.seed, save.generator, &assets_->catalog().models(),
                                         save.dimension, save.type);
    for (const auto& edit : save.edits)
        world->blocks.set(edit.position, edit.value);
    Player player;
    player.feet = save.feet;
    player.camera.yaw = save.yaw;
    player.camera.pitch = save.pitch;
    player.height = save.crouched ? 1.3f : 1.8f;
    if (!save.survival.dead() && collides(world->blocks, player, player.feet))
        throw Error("Saved player position intersects solid terrain");
    player.step(world->blocks, {}, 0, save.crouched);
    Inventory inventory;
    for (unsigned i = 0; i < ItemCount; ++i)
        inventory.add(static_cast<Item>(i), save.inventory[i]);
    inventory.choose(static_cast<Item>(save.selected));
    AnimalSystem animals;
    if (save.dimension == Dimension::Overworld)
        animals.spawn(world->blocks);
    {
        Frame frame;
        world->prime(player.feet);
    }
    world_ = std::move(world);
    animals_ = std::move(animals);
    player_ = player;
    inventory_ = inventory;
    cycle_.setSeconds(save.daySeconds);
    worldId_ = id;
    worldName_ = save.name;
    worldSeed_ = save.seed;
    session_.survival.fallTop = player_.feet.y;
    deathProcessed_ = session_.survival.dead();
    inventoryOpen_ = false;
    chestOpen_ = false;
    inventoryPage_ = save.selected / 27;
    inventoryCursor_ = save.selected % 27;
    state_ = STATE_GAMEPLAY;
    message_ =
        save.recovered ? "Recovered previous valid save" : "R break/attack | L place/interact";
}
void App::saveWorld(bool showProgress) {
    if (!world_)
        return;
    if (showProgress)
        renderer_.loading("Saving " + worldName_, 0, 1);
    WorldSave save = session_;
    save.name = worldName_;
    save.seed = worldSeed_;
    save.generator = world_->blocks.terrain().version();
    save.feet = player_.feet;
    save.yaw = player_.camera.yaw;
    save.pitch = player_.camera.pitch;
    save.crouched = player_.height < 1.8f;
    save.daySeconds = cycle_.seconds();
    save.selected = unsigned(inventory_.selected());
    for (unsigned i = 0; i < ItemCount; ++i)
        save.inventory[i] = inventory_.count(static_cast<Item>(i));
    save.edits.clear();
    for (const auto& edit : world_->blocks.edits())
        save.edits.push_back({edit.first, edit.second});
    saves_.save(worldId_, save);
}
void App::openEditor(bool seed) {
    editing_ = true;
    editingSeed_ = seed;
    key_ = 0;
    editor_.open(seed ? seed_ : name_, seed);
}
void App::nativeName() {
    { Frame drain; }
    SwkbdState keyboard;
    swkbdInit(&keyboard, SWKBD_TYPE_NORMAL, 2, 32);
    swkbdSetFeatures(&keyboard, SWKBD_DEFAULT_QWERTY);
    swkbdSetValidation(&keyboard, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
    swkbdSetInitialText(&keyboard, name_.c_str());
    swkbdSetHintText(&keyboard, "World name");
    char buffer[129]{};
    const auto button = swkbdInputText(&keyboard, buffer, sizeof(buffer));
    if (button == SWKBD_BUTTON_RIGHT)
        name_ = buffer;
    else if (button == SWKBD_BUTTON_NONE && swkbdGetResult(&keyboard) < 0)
        openEditor(false);
}
void App::travel(Dimension target) {
    if (target == session_.dimension)
        return;
    const auto current = unsigned(session_.dimension), next = unsigned(target);
    auto& old = session_.journals[current];
    old.clear();
    for (const auto& e : world_->blocks.edits())
        old.push_back({e.first, e.second});
    session_.arrivals[current] = player_.feet;
    Vec3 preferred = session_.visited[next] ? session_.arrivals[next] : Vec3{};
    if (!session_.visited[next] && target == Dimension::Nether)
        preferred = player_.feet * (1.0f / 8);
    if (!session_.visited[next] && target == Dimension::Overworld &&
        session_.dimension == Dimension::Nether)
        preferred = player_.feet * 8;
    renderer_.loading(std::string("Entering ") + dimensionName(target), 0, 1);
    // Release the old VBO window before allocating the destination's window.
    {
        Frame drain;
        world_->releaseMeshes();
    }
    auto world = std::make_unique<World>(worldSeed_, session_.generator,
                                         &assets_->catalog().models(), target, session_.type);
    for (const auto& e : session_.journals[next])
        world->blocks.set(e.position, e.value);
    Vec3 arrival;
    try {
        arrival = safeArrival(world->blocks, target, preferred);
    } catch (const std::runtime_error&) {
        arrival = safeArrival(world->blocks, target, {});
    }
    {
        Frame frame;
        world->prime(arrival);
    }
    if (target == Dimension::End && !session_.visited[next])
        inventory_.add(Item::Elytra);
    session_.visited[next] = true;
    session_.dimension = target;
    world_ = std::move(world);
    player_.feet = arrival;
    player_.verticalSpeed = 0;
    player_.height = 1.8f;
    player_.step(world_->blocks, {}, 0);
    inventoryOpen_ = false;
    chestOpen_ = false;
    session_.survival.gliding = false;
    session_.survival.fallTop = arrival.y;
    animals_ = AnimalSystem{};
    if (target == Dimension::Overworld)
        animals_.spawn(world_->blocks);
    state_ = STATE_GAMEPLAY;
    message_ = "Arrived safely | SELECT for travel and crafting";
}
void App::respawn() {
    inventoryOpen_ = false;
    chestOpen_ = false;
    if (session_.dimension != Dimension::Overworld)
        travel(Dimension::Overworld);
    const auto shield = session_.survival.shieldDurability,
               elytra = session_.survival.elytraDurability;
    session_.survival.respawn();
    session_.survival.shieldDurability = shield;
    session_.survival.elytraDurability = elytra;
    player_.feet = safeArrival(world_->blocks, Dimension::Overworld, {});
    player_.verticalSpeed = 0;
    player_.step(world_->blocks, {}, 0);
    session_.survival.fallTop = player_.feet.y;
    deathProcessed_ = false;
    message_ = "Respawned. Return to your death position to recover items.";
}
void App::updateSurvival(float dt, Vec3 previous) {
    auto& s = session_.survival;
    const auto moved = player_.feet - previous;
    s.tick(dt, session_.mode, std::sqrt(moved.x * moved.x + moved.z * moved.z));
    if (!s.gliding) {
        s.fallTop = std::max(s.fallTop, player_.feet.y);
        if (player_.grounded) {
            s.damage(std::floor(s.fallTop - player_.feet.y - 3), false, false, session_.mode);
            s.fallTop = player_.feet.y;
        }
    }
    if (player_.feet.y < -16)
        s.damage(100, false, false, session_.mode);
    if (session_.mode == GameMode::Creative && player_.feet.y < -16) {
        player_.feet = safeArrival(world_->blocks, session_.dimension, {});
        player_.verticalSpeed = 0;
    }
    const auto block = world_->blocks.block(int(std::floor(player_.feet.x)),
                                            int(std::floor(player_.feet.y - 0.1f)),
                                            int(std::floor(player_.feet.z)));
    if (block == Block::Lava)
        s.damage(4, false, false, session_.mode);
    hostileTime_ += dt;
    contactTime_ += dt;
    if (hostileTime_ >= 15 && session_.mode == GameMode::Survival) {
        hostileTime_ = 0;
        if (session_.dimension != Dimension::Overworld || cycle_.light().ambient < 0.2f)
            animals_.spawnHostile(world_->blocks, player_.feet, session_.dimension);
    }
    Vec3 enemy;
    if (contactTime_ >= 1 && animals_.threat(player_.feet, enemy)) {
        const auto direction = normalized(enemy - player_.feet);
        if (!raycast(world_->blocks, player_.camera.eye, direction, 1.5f).hit)
            s.damage(3, true, dot(direction, player_.camera.forward()) > 0, session_.mode);
        contactTime_ = 0;
    }
    if (s.dead() && !deathProcessed_) {
        session_.dropActive = true;
        session_.dropPosition = player_.feet;
        session_.dropDimension = session_.dimension;
        for (unsigned i = 0; i < ItemCount; ++i)
            session_.dropped[i] = inventory_.count(static_cast<Item>(i));
        if (s.offhand != Item::Count)
            session_.dropped[unsigned(s.offhand)] =
                std::min(Inventory::StackLimit, session_.dropped[unsigned(s.offhand)] + 1);
        if (s.elytra)
            session_.dropped[unsigned(Item::Elytra)] =
                std::min(Inventory::StackLimit, session_.dropped[unsigned(Item::Elytra)] + 1);
        for (auto& armor : s.armor) {
            if (armor != Item::Count)
                session_.dropped[unsigned(armor)] =
                    std::min(Inventory::StackLimit, session_.dropped[unsigned(armor)] + 1);
            armor = Item::Count;
        }
        s.armorWear.fill(0);
        inventory_ = Inventory{};
        s.offhand = Item::Count;
        s.elytra = false;
        s.gliding = false;
        deathProcessed_ = true;
    }
    if (!s.dead() && session_.dropActive && session_.dropDimension == session_.dimension &&
        dot(player_.feet - session_.dropPosition, player_.feet - session_.dropPosition) < 9) {
        bool left = false;
        for (unsigned i = 0; i < ItemCount; ++i) {
            auto& count = session_.dropped[i];
            const auto take =
                std::min(count, Inventory::StackLimit - inventory_.count(static_cast<Item>(i)));
            inventory_.add(static_cast<Item>(i), take);
            count -= take;
            left |= count > 0;
        }
        session_.dropActive = left;
        message_ = "Recovered dropped inventory";
    }
}
void App::menuInput(u32 down) {
    if (editing_) {
        const auto count = unsigned(editor_.keys().size());
        if (down & KEY_R) {
            editing_ = false;
            return;
        }
        if (down & KEY_DLEFT)
            key_ = (key_ + count - 1) % count;
        if (down & KEY_DRIGHT)
            key_ = (key_ + 1) % count;
        if (down & KEY_DUP)
            key_ = key_ >= 10 ? key_ - 10 : key_;
        if (down & KEY_DDOWN)
            key_ = std::min(count - 1, key_ + 10);
        bool activate = (down & KEY_L) != 0;
        if (down & KEY_TOUCH) {
            touchPosition touch;
            hidTouchRead(&touch);
            if (touch.px >= 10 && touch.px < 310 && touch.py >= 70 && touch.py < 210) {
                const unsigned key = (touch.py - 70) / 28 * 10 + (touch.px - 10) / 30;
                if (key < count) {
                    key_ = key;
                    activate = true;
                }
            }
        }
        if (activate && editor_.press(key_)) {
            if (editingSeed_) {
                parseSeed(editor_.text());
                seed_ = editor_.text();
            } else {
                if (editor_.text().empty())
                    throw Error("World name cannot be empty");
                name_ = editor_.text();
            }
            editing_ = false;
        }
        return;
    }
    if (down & KEY_START) {
        if (world_) {
            state_ = STATE_GAMEPLAY;
            return;
        }
        quit_ = true;
        return;
    }
    if (down & KEY_R) {
        state_ = world_ ? STATE_GAMEPLAY : STATE_MAIN_MENU;
        selected_ = 0;
        return;
    }
    unsigned count = state_ == STATE_MAIN_MENU      ? 3
                     : state_ == STATE_WORLD_SELECT ? unsigned(worlds_.size())
                     : state_ == STATE_CREATION     ? 7
                     : state_ == STATE_ACTIONS      ? 10
                     : state_ == STATE_FURNACE      ? 8
                     : state_ == STATE_CRAFTING     ? unsigned(recipes_.recipes().size())
                                                    : 4;
    if (count == 0)
        return;
    if (down & KEY_DUP)
        selected_ = (selected_ + count - 1) % count;
    if (down & KEY_DDOWN)
        selected_ = (selected_ + 1) % count;
    bool activate = (down & KEY_L) != 0;
    if ((down & KEY_TOUCH) && state_ != STATE_FURNACE) {
        touchPosition touch;
        hidTouchRead(&touch);
        if (touch.px >= 8 && touch.px < 312 && touch.py >= 63 && touch.py < 203) {
            const unsigned row = selected_ / 5 * 5 + (touch.py - 63) / 28;
            if (row < count) {
                selected_ = row;
                activate = true;
            }
        }
    }
    if (!activate)
        return;
    if (state_ == STATE_MAIN_MENU) {
        if (selected_ == 0) {
            worlds_ = saves_.list(progress());
            state_ = STATE_WORLD_SELECT;
        } else if (selected_ == 1)
            state_ = STATE_CREATION;
        else
            state_ = STATE_SETTINGS;
        selected_ = 0;
    } else if (state_ == STATE_WORLD_SELECT) {
        const auto& entry = worlds_.at(selected_);
        if (!entry.valid)
            throw Error("%s", entry.error.c_str());
        beginWorld(entry.id);
    } else if (state_ == STATE_ACTIONS) {
        if (selected_ == 0) {
            state_ = STATE_CRAFTING;
            selected_ = 0;
            return;
        }
        if (selected_ == 1)
            message_ = session_.survival.equipOffhand(inventory_, session_.survival.offhand ==
                                                                          inventory_.selected()
                                                                      ? Item::Count
                                                                      : inventory_.selected())
                           ? "Off-hand updated"
                           : "Item unavailable / bag full";
        if (selected_ == 2)
            message_ = session_.survival.equipElytra(inventory_) ? "Elytra equipment updated"
                                                                 : "Elytra unavailable / bag full";
        if (selected_ == 3) {
            if (inventory_.remove(Item::Leather, 2)) {
                session_.survival.elytraDurability = 432;
                message_ = "Elytra repaired";
            } else
                message_ = "Need 2 leather";
        }
        if (selected_ >= 4 && selected_ <= 6)
            travel(static_cast<Dimension>(selected_ - 4));
        if (selected_ == 8)
            message_ = session_.survival.equipArmor(inventory_, inventory_.selected())
                           ? "Armor updated"
                           : "Select armor / remove Elytra / free bag space";
        if (selected_ == 7) {
            debugMesh_ = !debugMesh_;
            message_ = debugMesh_ ? "Mesh diagnostic: solid geometry" : "Textured terrain enabled";
        }
        state_ = STATE_GAMEPLAY;
    } else if (state_ == STATE_FURNACE) {
        auto& furnace = session_.furnaces[unsigned(session_.dimension)].at(furnacePosition_);
        if (selected_ == 7) {
            state_ = STATE_GAMEPLAY;
            return;
        }
        bool changed = selected_ < 4 ? furnace.deposit(inventory_, inventory_.selected(),
                                                       selected_ >= 2, selected_ % 2 != 0)
                                     : furnace.take(inventory_, selected_ == 4 ? 2 : selected_ - 5);
        message_ = changed ? "Furnace updated" : "Unavailable / incompatible / full";
    } else if (state_ == STATE_CRAFTING) {
        message_ = recipes_.craft(recipes_.recipes().at(selected_), inventory_)
                       ? "Crafted"
                       : "Missing ingredients or inventory full";
    } else if (state_ == STATE_CREATION) {
        if (selected_ == 0)
            nativeName();
        else if (selected_ == 1) {
            if (nameRandom_ == 1)
                nameRandom_ = std::uint32_t(svcGetSystemTick());
            name_ = randomWorldName(nameRandom_);
        } else if (selected_ == 2)
            openEditor(true);
        else if (selected_ == 3) {
            creationMode_ =
                creationMode_ == GameMode::Survival ? GameMode::Creative : GameMode::Survival;
            if (creationMode_ == GameMode::Survival)
                creationType_ = WorldType::Normal;
        } else if (selected_ == 4) {
            if (creationMode_ == GameMode::Creative)
                creationType_ =
                    creationType_ == WorldType::Normal ? WorldType::Superflat : WorldType::Normal;
        } else if (selected_ == 5)
            openEditor(false);
        else {
            loadAssets();
            const auto id = saves_.create(name_, parseSeed(seed_), svcGetSystemTick(),
                                          creationMode_, creationType_);
            beginWorld(id);
        }
    } else if (state_ == STATE_SETTINGS) {
        if (selected_ == 0) {
            settings_.lookSpeed += 0.4f;
            if (settings_.lookSpeed > 3.5f)
                settings_.lookSpeed = 1.0f;
        }
        if (selected_ == 1) {
            settings_.deadzone += 4;
            if (settings_.deadzone > 31)
                settings_.deadzone = 7;
        }
        if (selected_ == 2)
            settings_.invertY = !settings_.invertY;
        if (selected_ == 3) {
            state_ = STATE_MAIN_MENU;
            selected_ = 0;
        }
    }
}
void App::drawMenu() {
    if (editing_) {
        renderer_.keyboard(editor_, key_, editingSeed_ ? "World seed" : "World name");
        return;
    }
    if (state_ == STATE_MAIN_MENU)
        renderer_.menu("3DS Craft", {{"Play"}, {"Create New World"}, {"Settings"}}, selected_,
                       "Bottom-screen edition", "L select | R back | START exit");
    else if (state_ == STATE_WORLD_SELECT) {
        std::vector<MenuRow> rows;
        for (const auto& world : worlds_)
            rows.push_back({world.name + (world.valid ? "" : " [invalid]"), world.valid});
        renderer_.menu("Select World", rows, selected_,
                       rows.empty() ? "No worlds yet. R returns to create one." : "SD card saves");
    } else if (state_ == STATE_CREATION)
        renderer_.menu("Create World",
                       {{"Name: " + name_},
                        {"Randomize name"},
                        {"Seed: " + seed_},
                        {creationMode_ == GameMode::Creative ? "Mode: Creative" : "Mode: Survival"},
                        {creationType_ == WorldType::Superflat ? "Type: Superflat" : "Type: Normal",
                         creationMode_ == GameMode::Creative},
                        {"Bottom keyboard"},
                        {"Create and Play"}},
                       selected_, "L select | D-pad scroll pages");
    else if (state_ == STATE_ACTIONS)
        renderer_.menu("Game actions",
                       {{"Craft items"},
                        {"Equip / remove off-hand"},
                        {"Equip / remove Elytra"},
                        {"Repair Elytra (2 leather)"},
                        {"Travel: Overworld"},
                        {"Travel: Nether"},
                        {"Travel: The End"},
                        {debugMesh_ ? "Terrain: solid diagnostic" : "Terrain: textured"},
                        {"Equip / remove selected armor"},
                        {"Resume"}},
                       selected_,
                       session_.dropActive
                           ? std::string("Dropped bag: ") + dimensionName(session_.dropDimension) +
                                 " " + std::to_string(int(session_.dropPosition.x)) + "," +
                                 std::to_string(int(session_.dropPosition.y)) + "," +
                                 std::to_string(int(session_.dropPosition.z))
                           : "L select | R resume");
    else if (state_ == STATE_CRAFTING) {
        std::vector<MenuRow> rows;
        for (const auto& r : recipes_.recipes())
            rows.push_back(
                {std::string(Inventory::name(r.result)) + " x" + std::to_string(r.resultCount)});
        std::string cost = "Need: ";
        for (const auto& i : recipes_.recipes().at(selected_).ingredients)
            if (i.item != Item::Count)
                cost += std::string(Inventory::name(i.item)) + " x" + std::to_string(i.count) + " ";
        renderer_.menu("Crafting", rows, selected_, cost, message_);
    } else if (state_ == STATE_FURNACE) {
        renderer_.furnace(session_.furnaces[unsigned(session_.dimension)].at(furnacePosition_),
                          *assets_, inventory_.selected(), selected_, message_);
    } else {
        char speed[64];
        std::snprintf(speed, sizeof(speed), "Look speed: %.1f", settings_.lookSpeed);
        renderer_.menu("Settings",
                       {{speed},
                        {"Dead zone: " + std::to_string(settings_.deadzone)},
                        {settings_.invertY ? "Invert Y: on" : "Invert Y: off"},
                        {"Back"}},
                       selected_, "L changes value (this session)");
    }
}
void App::gameplay(u32 down, u32 held, float dt, double elapsed) {
    if (down & KEY_START) {
        saveWorld();
        world_.reset();
        worldId_.clear();
        state_ = STATE_MAIN_MENU;
        selected_ = 0;
        return;
    }
    if ((down & KEY_SELECT) && !session_.survival.dead()) {
        state_ = STATE_ACTIONS;
        selected_ = 0;
        return;
    }
    if (session_.survival.dead() && (down & KEY_B)) {
        respawn();
        return;
    }
    metrics_.frame(elapsed);
    cycle_.advance(elapsed);
    if (down & (KEY_X | KEY_Y)) {
        inventoryOpen_ = !inventoryOpen_;
        chestOpen_ = false;
        inventoryPage_ = unsigned(inventory_.selected()) / 27;
        inventoryCursor_ = unsigned(inventory_.selected()) % 27;
    }
    const auto previousItem = inventory_.selected();
    if (inventoryOpen_ && (down & (KEY_ZL | KEY_ZR))) {
        inventoryPage_ =
            (inventoryPage_ + InventoryPages + ((down & KEY_ZL) ? -1 : 1)) % InventoryPages;
        if (inventoryPageItem(inventoryPage_, inventoryCursor_) == Item::Count)
            inventoryCursor_ = chestOpen_ && inventoryCursor_ >= 27 ? 27 : 0;
    } else if (!inventoryOpen_) {
        if (down & KEY_ZL)
            inventory_.select(-1);
        if (down & KEY_ZR)
            inventory_.select(1);
    }
    circlePosition move{}, look{};
    hidCircleRead(&move);
    hidCstickRead(&look);
    const auto input = mapAnalog({move.dx, move.dy, look.dx, look.dy}, settings_);
    Movement movement{};
    const bool dead = session_.survival.dead();
    const auto main = inventory_.count(inventory_.selected()) ? inventory_.selected() : Item::Count;
    if (dead) {
        movement = {};
    } else if (inventoryOpen_) {
        const unsigned slots = chestOpen_ ? 54 : 27;
        int direction = (down & KEY_DLEFT)    ? -1
                        : (down & KEY_DRIGHT) ? 1
                        : (down & KEY_DUP)    ? -9
                        : (down & KEY_DDOWN)  ? 9
                                              : 0;
        if (direction) {
            unsigned next = (inventoryCursor_ + slots + direction) % slots;
            for (unsigned n = 0;
                 n < slots && inventoryPageItem(inventoryPage_, next) == Item::Count; ++n)
                next = (next + slots + (direction < 0 ? -1 : 1)) % slots;
            inventoryCursor_ = next;
        }
        if (chestOpen_ && (down & (KEY_L | KEY_R)) &&
            inventoryPageItem(inventoryPage_, inventoryCursor_) != Item::Count) {
            auto& chest = session_.chests[unsigned(session_.dimension)].at(chestPosition_);
            auto item = inventoryPageItem(inventoryPage_, inventoryCursor_);
            bool deposit = inventoryCursor_ >= 27;
            message_ = transferChest(chest, inventory_, item, deposit, (down & KEY_R) != 0)
                           ? "Items transferred"
                           : "Stack empty or destination full";
        } else if ((down & KEY_L) &&
                   inventoryPageItem(inventoryPage_, inventoryCursor_) != Item::Count) {
            inventory_.choose(inventoryPageItem(inventoryPage_, inventoryCursor_));
            message_ = "Item selected";
        }
    } else {
        movement = {input.forward, input.strafe, (down & KEY_B) != 0};
        player_.camera.yaw =
            std::remainder(player_.camera.yaw + input.lookX * settings_.lookSpeed * dt, 2 * Pi);
        player_.camera.pitch =
            std::clamp(player_.camera.pitch + input.lookY * settings_.lookSpeed * dt, -1.5f, 1.5f);
    }
    auto& survival = session_.survival;
    if (inventory_.selected() != previousItem)
        survival.attackAge = 0;
    const bool mainUsesL =
        main != Item::Count && (ItemTypes[unsigned(main)].places != Block::Air ||
                                (foodStats(main).hunger > 0 && survival.hunger < 20));
    survival.blocking = !inventoryOpen_ && !dead && (held & KEY_L) &&
                        survival.offhand == Item::Shield && !mainUsesL;
    const auto previous = player_.feet;
    if (!dead) {
        if (!inventoryOpen_ && (down & KEY_B) && !player_.grounded && survival.elytra &&
            survival.elytraDurability > 1) {
            survival.gliding = !survival.gliding;
            survival.glideVelocity =
                player_.camera.forward() * 9 + Vec3{0, player_.verticalSpeed, 0};
        }
        if (!survival.glide(player_, world_->blocks, dt, session_.mode))
            player_.step(world_->blocks, movement, dt, (held & KEY_A) != 0 || survival.blocking);
        animals_.updateVillages(world_->blocks, player_.feet, dt,
                                assets_->catalog()
                                    .regions()
                                    .at(assets_->catalog().sprite("entity/villager/villager"))
                                    .uv);
        animals_.update(world_->blocks, player_.feet, dt);
        updateSurvival(dt, previous);
        if (chestOpen_) {
            Vec3 offset =
                player_.feet - Vec3{float(chestPosition_.x) + .5f, float(chestPosition_.y),
                                    float(chestPosition_.z) + .5f};
            if (survival.dead() || dot(offset, offset) > 36 ||
                world_->blocks.block(chestPosition_.x, chestPosition_.y, chestPosition_.z) !=
                    Block::Chest) {
                chestOpen_ = false;
                inventoryOpen_ = false;
            }
        }
    }
    if (!survival.dead() && !inventoryOpen_ && ((held & KEY_R) || (down & KEY_L))) {
        const auto hit = raycast(world_->blocks, player_.camera.eye, player_.camera.forward());
        const auto loaded = [&](BlockPos p) {
            const auto key = chunkOf(p);
            return world_->loaded(key.x, key.z);
        };
        bool attacked = false;
        if (down & KEY_R) {
            const bool sweep = toolStats(main).kind == ToolKind::Sword &&
                               survival.charge(main) > .9f && player_.grounded;
            const bool critical =
                !player_.grounded && player_.verticalSpeed < 0 && !survival.gliding;
            message_ = animals_.attack(world_->blocks, inventory_, player_.camera,
                                       survival.attack(main, critical), sweep);
            attacked = std::strcmp(message_, "No animal in reach") != 0;
            if (attacked && std::strcmp(message_, "Drop stack full") != 0 &&
                session_.mode == GameMode::Survival)
                survival.wearTool(inventory_, main,
                                  toolStats(main).kind == ToolKind::Sword ? 1 : 2);
        }
        if ((down & KEY_L) && !survival.blocking && survival.eat(inventory_, main)) {
            message_ = "Ate food";
            attacked = true;
        }
        if ((down & KEY_L) && !survival.blocking && !mainUsesL &&
            foodStats(survival.offhand).hunger > 0 && survival.hunger < 20) {
            Inventory food;
            food.add(survival.offhand);
            survival.eat(food, survival.offhand);
            survival.offhand = Item::Count;
            attacked = true;
            message_ = "Ate from off-hand";
        }
        if (!attacked && !survival.blocking) {
            if (!hit.hit) {
                message_ = "No block in reach";
                survival.mine({}, main, dt, session_.mode);
            } else if (!loaded(hit.position))
                message_ = "Wait for terrain to load";
            else if (held & KEY_R) {
                bool furnaceFull = false;
                if (hit.block == Block::Furnace) {
                    const auto& fs = session_.furnaces[unsigned(session_.dimension)];
                    auto at = fs.find(hit.position);
                    furnaceFull = at != fs.end() && !at->second.empty();
                }
                if (hit.block == Block::Chest || furnaceFull) {
                    auto& containers = session_.chests[unsigned(session_.dimension)];
                    auto it = containers.find(hit.position);
                    if (furnaceFull ||
                        (it != containers.end() && std::any_of(it->second.begin(), it->second.end(),
                                                               [](unsigned n) { return n > 0; }))) {
                        message_ = "Empty the container before mining";
                        survival.miningTime = 0;
                        Frame frame;
                        world_->update(player_.feet);
                        renderer_.draw(*world_, player_.camera, animals_, cycle_, inventory_,
                                       metrics_, message_, *assets_, false, 0, survival,
                                       session_.mode, session_.dimension, debugMesh_);
                        return;
                    }
                }
                if (!assets_->catalog().block(hit.block).breakable)
                    message_ = "This block cannot be broken";
                else if (survival.mine(hit, main, dt, session_.mode)) {
                    if (session_.mode == GameMode::Creative || !canHarvest(hit.block, main))
                        message_ = world_->blocks.set(hit.position, Block::Air)
                                       ? "Block removed"
                                       : "Edit limit reached";
                    else
                        message_ = breakBlock(world_->blocks, inventory_, hit);
                    if (session_.mode == GameMode::Survival &&
                        world_->blocks.block(hit.position.x, hit.position.y, hit.position.z) ==
                            Block::Air)
                        survival.wearTool(inventory_, main,
                                          toolStats(main).kind == ToolKind::Sword ? 2 : 1);
                    if (hit.block == Block::Furnace &&
                        world_->blocks.block(hit.position.x, hit.position.y, hit.position.z) ==
                            Block::Air)
                        session_.furnaces[unsigned(session_.dimension)].erase(hit.position);
                    if (hit.block == Block::Chest &&
                        world_->blocks.block(hit.position.x, hit.position.y, hit.position.z) ==
                            Block::Air)
                        session_.chests[unsigned(session_.dimension)].erase(hit.position);
                }
            } else if (hit.block == Block::CraftingTable) {
                state_ = STATE_CRAFTING;
                selected_ = 0;
            } else if (hit.block == Block::Furnace) {
                auto& furnaces = session_.furnaces[unsigned(session_.dimension)];
                if (!furnaces.count(hit.position) && furnaces.size() >= 64)
                    message_ = "Furnace limit reached";
                else {
                    furnaces.try_emplace(hit.position);
                    furnacePosition_ = hit.position;
                    state_ = STATE_FURNACE;
                    selected_ = 0;
                }
            } else if (assets_->catalog().block(hit.block).opensInventory) {
                auto& containers = session_.chests[unsigned(session_.dimension)];
                if (!containers.count(hit.position) && containers.size() >= 64)
                    message_ = "Chest storage limit reached";
                else {
                    containers.try_emplace(hit.position);
                    chestPosition_ = hit.position;
                    chestOpen_ = true;
                    inventoryOpen_ = true;
                    inventoryPage_ = unsigned(inventory_.selected()) / 27;
                    inventoryCursor_ = 27 + unsigned(inventory_.selected()) % 27;
                    message_ = "L transfer one | R transfer stack";
                }
            } else if (!loaded(hit.adjacent))
                message_ = "Wait for terrain to load";
            else {
                std::array<Aabb, AnimalSystem::MaxAnimals + 1> blockers;
                unsigned count = 0;
                blockers[count++] = player_.bounds();
                for (const auto& animal : animals_.animals())
                    blockers[count++] = animal->bounds();
                const auto selected = inventory_.selected();
                if (session_.mode == GameMode::Creative &&
                    Inventory::fromBlock(inventory_.selectedBlock()) != Item::Count) {
                    Inventory unlimited = inventory_;
                    unlimited.add(selected, 1);
                    message_ = placeBlock(world_->blocks, unlimited, hit, blockers.data(), count);
                } else if (!mainUsesL && survival.offhand != Item::Count &&
                           ItemTypes[unsigned(survival.offhand)].places != Block::Air) {
                    Inventory hand;
                    hand.add(survival.offhand);
                    hand.choose(survival.offhand);
                    message_ = placeBlock(world_->blocks, hand, hit, blockers.data(), count);
                    if (hand.count(survival.offhand) == 0)
                        survival.offhand = Item::Count;
                } else
                    message_ = placeBlock(world_->blocks, inventory_, hit, blockers.data(), count);
            }
        }
    }
    if (chestOpen_ && !inventoryOpen_)
        chestOpen_ = false;
    if (!(held & KEY_R)) {
        survival.miningTime = 0;
        survival.miningBlock = Block::Air;
    }
    Frame frame;
    world_->update(player_.feet);
    renderer_.draw(*world_, player_.camera, animals_, cycle_, inventory_, metrics_, message_,
                   *assets_, inventoryOpen_, inventoryCursor_, session_.survival, session_.mode,
                   session_.dimension, debugMesh_,
                   chestOpen_ && inventoryOpen_
                       ? &session_.chests[unsigned(session_.dimension)].at(chestPosition_)
                       : nullptr,
                   inventoryPage_);
}
void App::run() {
    try {
        loadAssets();
    } catch (const std::bad_alloc&) {
        throw;
    } catch (const std::exception& e) {
        renderer_.promptError(e.what());
    }
    u64 last = svcGetSystemTick();
    while (!quit_ && aptMainLoop()) {
        hidScanInput();
        const auto down = hidKeysDown(), held = hidKeysHeld();
        const u64 now = svcGetSystemTick();
        const double elapsed = double(now - last) / SYSCLOCK_ARM11;
        last = now;
        try {
            if (world_)
                for (auto& entry : session_.furnaces[unsigned(session_.dimension)]) {
                    auto key = chunkOf(entry.first);
                    if (world_->loaded(key.x, key.z) &&
                        world_->blocks.block(entry.first.x, entry.first.y, entry.first.z) ==
                            Block::Furnace)
                        entry.second.tick(float(elapsed));
                }
            if (state_ == STATE_GAMEPLAY)
                gameplay(down, held, std::min(0.05f, float(elapsed)), elapsed);
            else {
                menuInput(down);
                if (!quit_ && state_ != STATE_GAMEPLAY)
                    drawMenu();
            }
        } catch (const std::bad_alloc&) {
            throw;
        } catch (const std::exception& error) {
            if ((!world_ && !worldId_.empty()) || (state_ == STATE_GAMEPLAY && !(down & KEY_START)))
                throw;
            renderer_.promptError(error.what());
            last = svcGetSystemTick();
        }
    }
    if (world_)
        saveWorld(false); // Normal HOME/system exit; START gameplay saves before returning to menu.
}
} // namespace voxel

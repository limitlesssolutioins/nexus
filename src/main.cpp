#include "types.h"
#include "progression.h"
#include "persistence.h"
#include "drops.h"
#include "rift.h"
#include "combat.h"
#include "ui.h"

int main() {
    InitWindow(SCREEN_W, SCREEN_H, "Echoes of the Rift");
    SetTargetFPS(60);

    GameWorld world{};
    world.playerSprite = LoadTexture("assets/sprites/main.png");
    world.profile = LoadProfile("savegame.dat");
    world.player = MakePlayer(world.profile);
    
    // Check Tutorial Status (Force tutorial if unarmed or flag false)
    if (!world.profile.tutorialCompleted || world.profile.equippedIndices[0] == -1) {
        world.state = GameState::TUTORIAL_SELECT;
        world.profile.equippedIndices[0] = -1; // Ensure unarmed
    } else {
        world.state = GameState::HUB;
    }
    
    world.camera.offset = {SCREEN_W / 2.0f, SCREEN_H / 2.0f};
    world.camera.zoom = 1.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Player& pl = world.player;

        // === STATE INPUT ===
        if (world.state == GameState::HUB) {
            if (IsKeyPressed(KEY_ENTER)) {
                world.state = GameState::RIFT_SELECT;
                world.selectedRiftRank = std::min(world.profile.hunterRankLevel, 6);
            }
            if (IsKeyPressed(KEY_TAB)) world.state = GameState::SKILL_TREE;
            if (IsKeyPressed(KEY_C)) world.state = GameState::STATUS_WINDOW;
            if (IsKeyPressed(KEY_I)) world.state = GameState::INVENTORY;
            if (IsKeyPressed(KEY_M)) world.state = GameState::MARKET;
        }
        else if (world.state == GameState::RIFT_SELECT) {
            if (IsKeyPressed(KEY_ESCAPE)) world.state = GameState::HUB;
            if (IsKeyPressed(KEY_LEFT) && world.selectedRiftRank > 0) world.selectedRiftRank--;
            if (IsKeyPressed(KEY_RIGHT) && world.selectedRiftRank < std::min(world.profile.hunterRankLevel, 6)) world.selectedRiftRank++;
            if (IsKeyPressed(KEY_ENTER)) {
                world.rift = GenerateRift(world.selectedRiftRank);
                world.player = MakePlayer(world.profile);
                world.enemies.clear();
                world.projectiles.clear();
                world.drops.clear();
                SpawnWave(world);
                world.state = GameState::PLAYING;
            }
        }
        else if (world.state == GameState::PLAYING) {
            // Movement
            Vector2 move = {0, 0};
            if (IsKeyDown(KEY_W)) move.y -= 1;
            if (IsKeyDown(KEY_S)) move.y += 1;
            if (IsKeyDown(KEY_A)) move.x -= 1;
            if (IsKeyDown(KEY_D)) move.x += 1;
            
            if (Vector2Length(move) > 0) {
                Vector2 n = Vector2Scale(Vector2Normalize(move), pl.speed * dt);
                Vector2 next = Vector2Add(pl.position, n);
                bool col = false;
                for (const auto& o : world.rift.obstacles)
                    if (CheckCollisionCircleRec(next, pl.radius, o.rect)) { col = true; break; }
                if (!col) pl.position = next;

                // Update animation
                pl.frameTimer += dt;
                if (pl.frameTimer >= 0.15f) {
                    pl.frameTimer = 0;
                    pl.frame = (pl.frame + 1) % 5;
                }

                // Update direction
                if (move.y < 0) {
                    pl.direction = 2; // Back
                } else if (move.y > 0) {
                    pl.direction = 0; // Front
                } else if (move.x != 0) {
                    pl.direction = 1; // Side
                    pl.facingRight = (move.x > 0);
                }
            } else {
                pl.frame = 0; // Idle frame
            }
            
            pl.position.x = Clamp(pl.position.x, 20.0f, world.rift.worldWidth - 20.0f);
            pl.position.y = Clamp(pl.position.y, 80.0f, world.rift.worldHeight - 80.0f);

            // Dynamic Camera (Look Ahead)
            Vector2 mousePos = GetMousePosition();
            Vector2 center = { SCREEN_W / 2.0f, SCREEN_H / 2.0f };
            Vector2 offset = Vector2Subtract(mousePos, center);
            Vector2 targetPos = Vector2Add(pl.position, Vector2Scale(offset, 0.3f)); // Look 30% towards mouse
            
            // Smooth damp
            world.camera.target.x += (targetPos.x - world.camera.target.x) * 5.0f * dt;
            world.camera.target.y += (targetPos.y - world.camera.target.y) * 5.0f * dt;

            // Tool Switch
            if (IsKeyPressed(KEY_Q) || GetMouseWheelMove() != 0) {
                pl.miningMode = !pl.miningMode;
            }

            // Weapon Attack (LMB)
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && pl.fireTimer <= 0) {
                if (pl.miningMode) {
                    // Mining Mode
                    float rate = 0.4f / pl.attackSpeedMultiplier;
                    PerformMining(world);
                    pl.fireTimer = rate;
                } else {
                    // Combat Mode
                    // Get equipped weapon
                    int weaponIdx = world.profile.equippedIndices[(int)EquipSlot::WEAPON];
                    WeaponType wType = WeaponType::NONE;
                    float weaponDmg = 5.0f; // Base unarmed damage

                    if (weaponIdx >= 0 && weaponIdx < MAX_INVENTORY) {
                        wType = world.profile.inventory[weaponIdx].weaponType;
                        weaponDmg = world.profile.inventory[weaponIdx].attack;
                    }

                    float rate = pl.baseFireRate / pl.attackSpeedMultiplier;
                    
                    Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), world.camera);
                    Vector2 dir = Vector2Normalize(Vector2Subtract(mousePos, pl.position));

                    switch (wType) {
                        case WeaponType::SWORD: // Melee: Wide arc, medium range
                            rate = 0.5f / pl.attackSpeedMultiplier;
                            PerformMeleeAttack(world, 100, 120, weaponDmg * 1.2f, 20); // 120% dmg
                            pl.fireTimer = rate;
                            // VFX slash
                            world.vfx.push_back({Vector2Add(pl.position, Vector2Scale(dir, 40)), 40, 100, 0.2f, 0.2f, WHITE, true});
                            break;
                        case WeaponType::DAGGER: // Melee: Fast, narrow arc, low dmg
                            rate = 0.25f / pl.attackSpeedMultiplier;
                            PerformMeleeAttack(world, 70, 60, weaponDmg * 0.7f, 5); // 70% dmg
                            pl.fireTimer = rate;
                            world.vfx.push_back({Vector2Add(pl.position, Vector2Scale(dir, 30)), 20, 50, 0.1f, 0.1f, GRAY, true});
                            break;
                        case WeaponType::MACE: // Melee: Slow, heavy hit, high knockback
                            rate = 0.8f / pl.attackSpeedMultiplier;
                            PerformMeleeAttack(world, 90, 90, weaponDmg * 1.5f, 50); // 150% dmg
                            pl.fireTimer = rate;
                            world.vfx.push_back({Vector2Add(pl.position, Vector2Scale(dir, 45)), 50, 120, 0.3f, 0.3f, GOLD, true});
                            break;
                        case WeaponType::BOW: // Ranged: Physical arrow
                            rate = 0.6f / pl.attackSpeedMultiplier;
                            pl.fireTimer = rate;
                            world.projectiles.push_back({pl.position, Vector2Scale(dir, 700.0f), 4, weaponDmg, false, false, false, true});
                            break;
                        case WeaponType::STAFF: // Ranged: Magic bolt (slow, high dmg)
                        case WeaponType::ORB:   // Ranged: Magic orb
                        case WeaponType::NONE:  // Default magic bolt
                            rate = 0.5f / pl.attackSpeedMultiplier;
                            pl.fireTimer = rate;
                            world.projectiles.push_back({pl.position, Vector2Scale(dir, 500.0f), 6, weaponDmg, true, false, false, true});
                            break;
                        default: break;
                    }
                }
            }
            pl.fireTimer -= dt;

            // Ability keys 1-6
            for (int i = 0; i < MAX_ABILITY_SLOTS; i++) {
                if (pl.slots[i].abilityId >= 0) {
                    pl.slots[i].cooldownTimer -= dt;
                    if (pl.slots[i].cooldownTimer < 0) pl.slots[i].cooldownTimer = 0;
                    if (IsKeyPressed(KEY_ONE + i) && pl.slots[i].cooldownTimer <= 0)
                        ExecuteAbility(world, i);
                }
            }

            // Update all systems (FIXED: all calls present)
            UpdatePassives(pl, dt);
            UpdateEnemies(world, dt);
            UpdateEnemyDebuffs(world, dt);
            ProcessCollisions(world);
            UpdateProjectiles(world.projectiles, dt, world.rift.worldWidth, world.rift.worldHeight);
            UpdateVFX(world.vfx, dt);
            UpdateParticles(world.particles, dt);
            SpawnAmbientParticles(world.particles, world.rift.theme, dt);
            UpdateDrops(world.drops, pl, world.profile, dt);
            UpdateMaterialDrops(world.materialDrops, pl, world.profile, dt);
            UpdateRiftProgress(world, dt);
            UpdateRiftFog(world.rift, pl.position);
        }
        else if (world.state == GameState::SKILL_TREE) {
            if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_ESCAPE)) world.state = GameState::HUB;
            // Discipline tab clicks
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 m = GetMousePosition();
                for (int d = 0; d < DISCIPLINE_COUNT; d++) {
                    if (CheckCollisionPointRec(m, {30 + d * 208.0f, 75, 198, 30})) {
                        world.selectedDiscipline = d;
                    }
                }
                // Ability clicks
                int ids[ABILITIES_PER_DISCIPLINE];
                GetDisciplineAbilities((Discipline)world.selectedDiscipline, ids);
                for (int a = 0; a < ABILITIES_PER_DISCIPLINE; a++) {
                    Rectangle r = {60, 120 + a * 80.0f, (float)SCREEN_W - 120, 72};
                    if (CheckCollisionPointRec(m, r)) {
                        int id = ids[a];
                        if (!world.profile.unlockedAbilities[id]) {
                            TryUnlockAbility(world.profile, pl, id);
                        } else {
                            // Toggle equip/unequip
                            int eqAt = -1;
                            for (int i = 0; i < MAX_ABILITY_SLOTS; i++)
                                if (world.profile.equippedSlots[i].abilityId == id) eqAt = i;
                            if (eqAt != -1) {
                                UnequipSlot(world.profile, pl, eqAt);
                            } else {
                                // Find first empty slot
                                int s = -1;
                                for (int i = 0; i < MAX_ABILITY_SLOTS; i++)
                                    if (world.profile.equippedSlots[i].abilityId < 0) { s = i; break; }
                                if (s != -1) EquipAbility(world.profile, pl, id, s);
                            }
                        }
                        SaveProfile("savegame.dat", world.profile);
                    }
                }
            }
        }
        else if (world.state == GameState::STATUS_WINDOW) {
            if (IsKeyPressed(KEY_C) || IsKeyPressed(KEY_ESCAPE)) {
                world.state = GameState::HUB;
                // Sync stats to profile
                world.profile.permanentStats = pl.stats;
                RecalculateStats(pl, world.profile);
                SaveProfile("savegame.dat", world.profile);
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && pl.stats.unspentStatPoints > 0) {
                for (int i = 0; i < STAT_COUNT; i++) {
                    Rectangle btn = {380, 150 + i * 55.0f - 2, 28, 28};
                    if (CheckCollisionPointRec(GetMousePosition(), btn)) {
                        pl.stats.AddByIndex(i, 1);
                        pl.stats.unspentStatPoints--;
                        world.profile.permanentStats = pl.stats;
                        RecalculateStats(pl, world.profile);
                    }
                }
            }
        }
        else if (world.state == GameState::INVENTORY) {
            if (IsKeyPressed(KEY_I) || IsKeyPressed(KEY_ESCAPE)) {
                world.state = GameState::HUB;
                SaveProfile("savegame.dat", world.profile);
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                // Check equipped slot clicks (unequip)
                for (int i = 0; i < EQUIP_SLOT_COUNT; i++) {
                    Rectangle r = {30, 90 + i * 70.0f, 280, 62};
                    if (CheckCollisionPointRec(GetMousePosition(), r) && world.profile.equippedIndices[i] >= 0) {
                        UnequipItem(world.profile, pl, (EquipSlot)i);
                    }
                }
                // Check inventory item clicks (equip)
                int col = 0, row = 0;
                for (int i = 0; i < MAX_INVENTORY; i++) {
                    if (!world.profile.inventory[i].active) continue;
                    float x = 350 + col * 300;
                    float y = 90 + row * 55.0f;
                    if (y > SCREEN_H - 80) break;
                    Rectangle r = {x, y, 290, 50};
                    if (CheckCollisionPointRec(GetMousePosition(), r)) {
                        EquipItem(world.profile, pl, i);
                    }
                    col++;
                    if (col >= 3) { col = 0; row++; }
                    if (col == 0) row++;
                }
            }
            // Right click to sell
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                int col = 0, row = 0;
                for (int i = 0; i < MAX_INVENTORY; i++) {
                    if (!world.profile.inventory[i].active) continue;
                    float x = 350 + col * 300;
                    float y = 90 + row * 55.0f;
                    if (y > SCREEN_H - 80) break;
                    Rectangle r = {x, y, 290, 50};
                    if (CheckCollisionPointRec(GetMousePosition(), r)) {
                        // Don't sell equipped items
                        bool equipped = false;
                        for (int s = 0; s < EQUIP_SLOT_COUNT; s++)
                            if (world.profile.equippedIndices[s] == i) equipped = true;
                        if (!equipped) {
                            world.profile.money += world.profile.inventory[i].sellPrice;
                            world.profile.inventory[i].active = false;
                        }
                    }
                    col++;
                    if (col >= 3) { col = 0; row++; }
                    if (col == 0) row++;
                }
            }
        }
        else if (world.state == GameState::MARKET) {
            if (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_ESCAPE)) world.state = GameState::HUB;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                // ... market logic ...
            }
        }
        else if (world.state == GameState::TUTORIAL_SELECT) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 m = GetMousePosition();
                // Option 1: Sword
                if (CheckCollisionPointRec(m, {100, 200, 300, 400})) {
                    Equipment sword{};
                    strcpy(sword.name, "Training Sword");
                    sword.slot = EquipSlot::WEAPON; sword.rarity = Rarity::COMMON; sword.weaponType = WeaponType::SWORD;
                    sword.attack = 12; sword.active = true;
                    world.profile.inventory[0] = sword;
                    world.profile.equippedIndices[(int)EquipSlot::WEAPON] = 0;
                    world.profile.tutorialCompleted = true;
                    world.state = GameState::HUB;
                    SaveProfile("savegame.dat", world.profile);
                }
                // Option 2: Bow
                if (CheckCollisionPointRec(m, {490, 200, 300, 400})) {
                    Equipment bow{};
                    strcpy(bow.name, "Old Hunter Bow");
                    bow.slot = EquipSlot::WEAPON; bow.rarity = Rarity::COMMON; bow.weaponType = WeaponType::BOW;
                    bow.attack = 10; bow.active = true;
                    world.profile.inventory[0] = bow;
                    world.profile.equippedIndices[(int)EquipSlot::WEAPON] = 0;
                    world.profile.tutorialCompleted = true;
                    world.state = GameState::HUB;
                    SaveProfile("savegame.dat", world.profile);
                }
                // Option 3: Staff
                if (CheckCollisionPointRec(m, {880, 200, 300, 400})) {
                    Equipment staff{};
                    strcpy(staff.name, "Novice Staff");
                    staff.slot = EquipSlot::WEAPON; staff.rarity = Rarity::COMMON; staff.weaponType = WeaponType::STAFF;
                    staff.attack = 15; staff.active = true;
                    world.profile.inventory[0] = staff;
                    world.profile.equippedIndices[(int)EquipSlot::WEAPON] = 0;
                    world.profile.tutorialCompleted = true;
                    world.state = GameState::HUB;
                    SaveProfile("savegame.dat", world.profile);
                }
            }
        }
        else if (world.state == GameState::TUTORIAL_SELECT) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 m = GetMousePosition();
                // Option 1: Sword
                if (CheckCollisionPointRec(m, {100, 200, 300, 400})) {
                    Equipment sword{};
                    strcpy(sword.name, "Training Sword");
                    sword.slot = EquipSlot::WEAPON; sword.rarity = Rarity::COMMON; sword.weaponType = WeaponType::SWORD;
                    sword.attack = 12; sword.active = true;
                    world.profile.inventory[0] = sword;
                    world.profile.equippedIndices[(int)EquipSlot::WEAPON] = 0;
                    world.profile.tutorialCompleted = true;
                    world.state = GameState::HUB;
                    SaveProfile("savegame.dat", world.profile);
                }
                // Option 2: Bow
                if (CheckCollisionPointRec(m, {490, 200, 300, 400})) {
                    Equipment bow{};
                    strcpy(bow.name, "Old Hunter Bow");
                    bow.slot = EquipSlot::WEAPON; bow.rarity = Rarity::COMMON; bow.weaponType = WeaponType::BOW;
                    bow.attack = 10; bow.active = true;
                    world.profile.inventory[0] = bow;
                    world.profile.equippedIndices[(int)EquipSlot::WEAPON] = 0;
                    world.profile.tutorialCompleted = true;
                    world.state = GameState::HUB;
                    SaveProfile("savegame.dat", world.profile);
                }
                // Option 3: Staff
                if (CheckCollisionPointRec(m, {880, 200, 300, 400})) {
                    Equipment staff{};
                    strcpy(staff.name, "Novice Staff");
                    staff.slot = EquipSlot::WEAPON; staff.rarity = Rarity::COMMON; staff.weaponType = WeaponType::STAFF;
                    staff.attack = 15; staff.active = true;
                    world.profile.inventory[0] = staff;
                    world.profile.equippedIndices[(int)EquipSlot::WEAPON] = 0;
                    world.profile.tutorialCompleted = true;
                    world.state = GameState::HUB;
                    SaveProfile("savegame.dat", world.profile);
                }
            }
        }
        else if (world.state == GameState::RIFT_COMPLETE || world.state == GameState::GAME_OVER) {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_R)) {
                // Save on exit
                SaveProfile("savegame.dat", world.profile);
                RefreshMissions(world.profile);
                world.enemies.clear();
                world.projectiles.clear();
                world.drops.clear();
                world.vfx.clear();
                world.particles.clear();
                world.player = MakePlayer(world.profile);
                world.state = GameState::HUB;
            }
        }

        // === RENDERING ===
        BeginDrawing();
        if (world.state == GameState::HUB) {
            DrawHub(world);
        }
        else if (world.state == GameState::RIFT_SELECT) {
            DrawRiftSelectUI(world);
        }
        else if (world.state == GameState::MARKET) {
            DrawMarketUI(world);
        }
        else if (world.state == GameState::PLAYING ||
                 world.state == GameState::RIFT_COMPLETE ||
                 world.state == GameState::GAME_OVER) {
            ClearBackground(world.rift.theme.bgColor);
            BeginMode2D(world.camera);
                DrawArena(world.rift, world.particles);
                
                // Render Obstacles with Depth Effect
                for (const auto& o : world.rift.obstacles) {
                    // Drop Shadow
                    DrawRectangleRec({o.rect.x + 6, o.rect.y + 6, o.rect.width, o.rect.height}, {10, 10, 20, 150});
                    
                    // Main Wall Body
                    DrawRectangleRec(o.rect, o.color);
                    
                    // Top Highlight (Inner Border)
                    DrawRectangleLinesEx(o.rect, 2, Fade(WHITE, 0.2f));
                    
                    // Side shading for a bit of volume
                    DrawRectangle(o.rect.x, o.rect.y + o.rect.height - 4, o.rect.width, 4, Fade(BLACK, 0.3f));
                }

                DrawEntities(world);
            EndMode2D();
            DrawHUD(world);
            if (world.state == GameState::RIFT_COMPLETE) DrawRiftComplete(world.rift);
            if (world.state == GameState::GAME_OVER) DrawGameOver(pl.killCount);
            if (world.state == GameState::PLAYING) DrawMinimap(world.rift, pl.position);
        }
        else {
            // Overlay screens (Skill Tree, Status, Inventory) drawn over rift or black bg
            ClearBackground({5, 5, 15, 255});
            if (world.state == GameState::SKILL_TREE) DrawSkillTreeUI(world);
            if (world.state == GameState::STATUS_WINDOW) DrawStatusWindow(world);
            if (world.state == GameState::INVENTORY) DrawInventoryUI(world);
            if (world.state == GameState::TUTORIAL_SELECT) {
                DrawText("CHOOSE YOUR PATH", SCREEN_W / 2 - 150, 80, 40, GOLD);
                DrawText("Select your starting weapon", SCREEN_W / 2 - 120, 130, 20, LIGHTGRAY);
                
                // Card 1: Warrior
                Rectangle r1 = {100, 200, 300, 400};
                bool h1 = CheckCollisionPointRec(GetMousePosition(), r1);
                DrawRectangleRec(r1, h1 ? Color{50, 30, 30, 255} : Color{30, 20, 20, 255});
                DrawRectangleLinesEx(r1, 3, h1 ? RED : DARKGRAY);
                DrawText("WARRIOR", 190, 230, 30, RED);
                DrawText("Weapon: Sword", 180, 280, 20, WHITE);
                DrawText("Balanced melee combat.", 160, 320, 18, GRAY);
                DrawText("High HP & Defense.", 160, 350, 18, GRAY);

                // Card 2: Hunter
                Rectangle r2 = {490, 200, 300, 400};
                bool h2 = CheckCollisionPointRec(GetMousePosition(), r2);
                DrawRectangleRec(r2, h2 ? Color{30, 50, 30, 255} : Color{20, 30, 20, 255});
                DrawRectangleLinesEx(r2, 3, h2 ? GREEN : DARKGRAY);
                DrawText("HUNTER", 590, 230, 30, GREEN);
                DrawText("Weapon: Bow", 580, 280, 20, WHITE);
                DrawText("Long range precision.", 550, 320, 18, GRAY);
                DrawText("High Speed & Crit.", 550, 350, 18, GRAY);

                // Card 3: Mage
                Rectangle r3 = {880, 200, 300, 400};
                bool h3 = CheckCollisionPointRec(GetMousePosition(), r3);
                DrawRectangleRec(r3, h3 ? Color{30, 30, 60, 255} : Color{20, 20, 40, 255});
                DrawRectangleLinesEx(r3, 3, h3 ? SKYBLUE : DARKGRAY);
                DrawText("MAGE", 990, 230, 30, SKYBLUE);
                DrawText("Weapon: Staff", 970, 280, 20, WHITE);
                DrawText("Powerful magic spells.", 940, 320, 18, GRAY);
                DrawText("Mana management.", 940, 350, 18, GRAY);
            }
        }
        EndDrawing();
    }

    SaveProfile("savegame.dat", world.profile);
    CloseWindow();
    return 0;
}

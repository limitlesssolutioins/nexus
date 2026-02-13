#include "ui.h"
#include "progression.h"
#include "combat.h"

void DrawArena(const Rift& rift, const std::vector<Particle>& particles) {
    const RiftTheme& t = rift.theme;
    
    // Draw Decorations (Floor Layer)
    for (const auto& d : rift.decorations) {
        if (d.type == 0) { // Rock
            DrawCircleV(d.position, 12 * d.scale, {80, 80, 80, 255});
            DrawCircleV(Vector2Add(d.position, {-3, -3}), 10 * d.scale, {100, 100, 100, 255});
        } else if (d.type == 1) { // Bones
            DrawLineEx(Vector2Subtract(d.position, {8, 8}), Vector2Add(d.position, {8, 8}), 3, RAYWHITE);
            DrawLineEx(Vector2Subtract(d.position, {8, -8}), Vector2Add(d.position, {8, -8}), 3, RAYWHITE);
        } else if (d.type == 2) { // Grass
            DrawCircleV(d.position, 8 * d.scale, {40, 100, 40, 100});
            DrawCircleLinesV(d.position, 9 * d.scale, {60, 140, 60, 120});
        } else if (d.type == 3) { // Torch
            // Base
            DrawCircleV(d.position, 5, BROWN);
            // Flame flickering
            float flicker = (float)GetRandomValue(8, 14);
            DrawCircleV(d.position, flicker, {255, 150, 0, 100}); // Glow
            DrawCircleV(d.position, flicker * 0.6f, {255, 200, 50, 200}); // Core
        }
    }

    // Draw particles
    for (const auto& p : particles) {
        if (!p.active) continue;
        float alpha = p.lifetime / p.maxLifetime;
        Color c = p.color;
        c.a = (unsigned char)(c.a * alpha);
        DrawCircleV(p.position, p.size, c);
    }

    // Draw Resource Nodes
    for (const auto& res : rift.resourceNodes) {
        if (!res.active) continue;
        Color c = (res.value > 1) ? Color{50, 255, 255, 255} : Color{50, 150, 200, 255}; // Cyan vs Blue
        DrawCircleV(res.position, 15, c);
        DrawCircleLinesV(res.position, 18, WHITE);
        // Little sparkles
        if (GetRandomValue(0, 100) < 5) 
            DrawPixel(res.position.x + GetRandomValue(-10, 10), res.position.y + GetRandomValue(-10, 10), WHITE);
    }
}

void DrawEntities(const GameWorld& world) {
    const Player& pl = world.player;

    // Enemies
    for (const auto& e : world.enemies) {
        if (!e.active) continue;
        Color ec = EnemyColor(e.type, world.rift.theme.enemyTint);
        if (e.state == EnemyState::ATTACKING) ec = WHITE;
        if (e.immobilized) ec = {100, 100, 200, 255};

        // Animation: Squash & Stretch
        float time = (float)GetTime() * 10.0f;
        float stretchX = 1.0f + sinf(time + e.position.x) * 0.05f;
        float stretchY = 1.0f + cosf(time + e.position.y) * 0.05f;
        if (e.state == EnemyState::CHASING) {
            stretchX = 1.0f + sinf(time * 1.5f) * 0.1f; // Faster wobble when chasing
            stretchY = 1.0f - sinf(time * 1.5f) * 0.1f;
        }

        // Vision cone
        float baseAngle = atan2f(e.facing.y, e.facing.x) * RAD2DEG;
        DrawCircleSectorLines(e.position, e.visionRange, baseAngle - 30, baseAngle + 30, 10, {ec.r, ec.g, ec.b, 50});

        // Draw body
        DrawEllipse((int)e.position.x, (int)e.position.y, e.radius * stretchX, e.radius * stretchY, ec);

        // HP bar for non-swarm
        if (e.type != EnemyType::SWARM) {
            float barW = e.radius * 2;
            float hpRatio = e.hp / e.maxHp;
            DrawRectangle((int)(e.position.x - barW/2), (int)(e.position.y - e.radius - 12), (int)barW, 6, DARKGRAY);
            DrawRectangle((int)(e.position.x - barW/2), (int)(e.position.y - e.radius - 12), (int)(barW * hpRatio), 6, RED);
        }

        // Alert indicator
        if (e.alerting) DrawText("!", (int)e.position.x - 5, (int)e.position.y - 40, 30, RED);

        // Elite/Boss markers
        if (e.type == EnemyType::ELITE) DrawText("E", (int)e.position.x - 5, (int)e.position.y - 8, 16, GOLD);
        if (e.type == EnemyType::BOSS) DrawText("BOSS", (int)e.position.x - 20, (int)e.position.y - 12, 18, RED);
    }

    // Projectiles
    for (const auto& p : world.projectiles) {
        if (!p.active) continue;
        Color c = p.isEnemy ? RED : (p.isMagic ? Color{120, 180, 255, 255} : GOLD);
        DrawCircleV(p.position, p.radius, c);
    }

    // Equipment drops
    for (const auto& d : world.drops) {
        if (!d.active) continue;
        Color c = RarityColor(d.item.rarity);
        DrawCircleV(d.position, 10, c);
        DrawCircleLinesV(d.position, 12, c);
    }

    // Material drops
    for (const auto& m : world.materialDrops) {
        if (!m.active) continue;
        Color c = (m.type == 0) ? Color{0, 255, 255, 255} : Color{200, 120, 255, 255};
        DrawCircleV(m.position, 8, c);
        DrawCircleLinesV(m.position, 10, WHITE);
        // Add a bit of glow
        DrawCircleGradient((int)m.position.x, (int)m.position.y, 15, Fade(c, 0.3f), Fade(c, 0));
    }

    // Summons
    for (int i = 0; i < MAX_SUMMONS; i++) {
        const Summon& s = pl.summons[i];
        if (!s.active) continue;
        Color sc = {150, 200, 255, 200};
        if (s.summonType == 1) sc = {200, 200, 100, 200}; // Guardian
        if (s.summonType == 2) sc = {100, 100, 180, 200}; // Shadow
        DrawCircleV(s.position, s.radius, sc);
        DrawCircleLinesV(s.position, s.radius + 2, WHITE);
    }

    // VFX
    for (const auto& v : world.vfx) {
        if (!v.active) continue;
        Color c = v.color;
        c.a = (unsigned char)(c.a * (v.lifetime / v.maxLifetime));
        DrawCircleLinesV(v.position, v.radius, c);
    }

    // Player
    if (world.playerSprite.id > 0) {
        float fw = (float)world.playerSprite.width / 5.0f;
        float fh = (float)world.playerSprite.height / 3.0f;
        Rectangle src = {(float)pl.frame * fw, (float)pl.direction * fh, fw, fh};
        if (!pl.facingRight) {
            src.width *= -1;
        }
        Rectangle dest = {pl.position.x, pl.position.y, pl.radius * 2.5f, pl.radius * 2.5f};
        Vector2 origin = {dest.width / 2, dest.height / 2};

        DrawTexturePro(world.playerSprite, src, dest, origin, 0, WHITE);
    } else {
        // Texture failed to load, draw a fallback circle
        Color playerColor = (pl.dmgCooldown > 0) ? RED : SKYBLUE;
        if (pl.shieldHp > 0) playerColor = {100, 200, 255, 255};
        DrawCircleV(pl.position, pl.radius, playerColor);
    }

    // Draw shield effect regardless
    if (pl.shieldHp > 0) {
        DrawCircleLinesV(pl.position, pl.radius + 5, {200, 230, 255, 180});
    }
}

void DrawHUD(const GameWorld& world) {
    const Player& pl = world.player;

    // Top-left info panel
    DrawRectangle(10, 10, 380, 110, {0, 0, 0, 180});
    // HP bar
    float hpRatio = pl.hp / pl.maxHp;
    DrawRectangle(25, 25, 200, 16, {40, 40, 40, 255});
    DrawRectangle(25, 25, (int)(200 * hpRatio), 16, (pl.hp < pl.maxHp * 0.3f) ? RED : GREEN);
    DrawText(TextFormat("HP: %.0f/%.0f", pl.hp, pl.maxHp), 235, 24, 16, WHITE);
    // Level + XP
    DrawText(TextFormat("Lv.%d  XP: %d/%d", pl.level, pl.xp, pl.xpToNext), 25, 48, 16, SKYBLUE);
    // Hunter Rank + Credits
    DrawText(TextFormat("Rank: %s | Credits: %d", RankName((Rank)world.profile.hunterRankLevel), world.profile.money), 25, 70, 16, GOLD);
    // Room info
    DrawText(TextFormat("Room %d/%d  Enemies: %d", world.rift.currentRoom + 1, world.rift.roomCount, world.rift.enemiesAlive), 25, 92, 14, LIGHTGRAY);

    // BOSS HEALTH BAR
    for (const auto& e : world.enemies) {
        if (e.active && e.type == EnemyType::BOSS) {
            float barW = 600;
            float barH = 20;
            float x = (SCREEN_W - barW) / 2;
            float y = 40;
            float hpRatio = e.hp / e.maxHp;
            
            DrawText("DUNGEON BOSS", (int)x + 240, (int)y - 25, 20, RED);
            DrawRectangle((int)x, (int)y, (int)barW, (int)barH, BLACK);
            DrawRectangle((int)x, (int)y, (int)(barW * hpRatio), (int)barH, RED);
            DrawRectangleLinesEx({x, y, barW, barH}, 2, GOLD);
            DrawText(TextFormat("%.0f / %.0f", e.hp, e.maxHp), (int)x + 260, (int)y + 2, 14, WHITE);
            break; // Only one boss bar
        }
    }

    // Shield bar (if active)
    if (pl.shieldHp > 0) {
        DrawRectangle(25, 44, (int)(200 * (pl.shieldHp / (pl.baseMaxHp * 0.5f))), 3, {100, 200, 255, 200});
    }

    // Ability slots (bottom center) — 6 slots
    for (int i = 0; i < MAX_ABILITY_SLOTS; i++) {
        float x = SCREEN_W / 2 - 185 + i * 62;
        AbilitySlot s = pl.slots[i];
        bool active = s.abilityId >= 0;
        DrawRectangleRec({x, (float)SCREEN_H - 70, 54, 54}, {0, 0, 0, 200});
        DrawRectangleLinesEx({x, (float)SCREEN_H - 70, 54, 54}, 2, active ? GetAbility(s.abilityId).color : DARKGRAY);
        if (active) {
            if (s.cooldownTimer > 0) {
                float cdRatio = s.cooldownTimer / s.cooldownMax;
                DrawRectangle((int)x, SCREEN_H - 70, 54, (int)(54 * cdRatio), {0, 0, 0, 160});
            }
            // Ability name (truncated)
            const char* name = GetAbility(s.abilityId).name;
            DrawText(TextFormat("%d", i + 1), (int)x + 3, SCREEN_H - 67, 12, WHITE);
            // Draw first 4 chars of ability name
            char shortName[6] = {};
            strncpy(shortName, name, 5);
            DrawText(shortName, (int)x + 3, SCREEN_H - 30, 10, LIGHTGRAY);
        } else {
            DrawText(TextFormat("%d", i + 1), (int)x + 20, SCREEN_H - 55, 18, DARKGRAY);
        }
    }

    // Active buffs indicator (top right)
    int buffCount = 0;
    const char* buffNames[] = {"Regen", "Shield", "Steal", "Reduce", "WarCry", "Iron", "Evasion", ""};
    for (int i = 0; i < MAX_BUFFS; i++) {
        if (pl.buffTimers[i] > 0) {
            DrawText(TextFormat("%s %.1fs", buffNames[i], pl.buffTimers[i]), SCREEN_W - 180, 15 + buffCount * 18, 14, GREEN);
            buffCount++;
        }
    }

    // Tool Mode Indicator (Bottom Right)
    DrawRectangle(SCREEN_W - 100, SCREEN_H - 100, 80, 80, {0, 0, 0, 200});
    DrawRectangleLines(SCREEN_W - 100, SCREEN_H - 100, 80, 80, GOLD);
    if (pl.miningMode) {
        DrawText("PICKAXE", SCREEN_W - 90, SCREEN_H - 90, 10, LIGHTGRAY);
        // Draw Pickaxe Icon (Lines)
        DrawLineEx({(float)SCREEN_W - 80, (float)SCREEN_H - 30}, {(float)SCREEN_W - 40, (float)SCREEN_H - 70}, 4, BROWN); // Handle
        DrawLineEx({(float)SCREEN_W - 50, (float)SCREEN_H - 75}, {(float)SCREEN_W - 30, (float)SCREEN_H - 55}, 6, GRAY); // Head
    } else {
        DrawText("WEAPON", SCREEN_W - 90, SCREEN_H - 90, 10, LIGHTGRAY);
        // Draw Sword Icon
        DrawLineEx({(float)SCREEN_W - 80, (float)SCREEN_H - 30}, {(float)SCREEN_W - 30, (float)SCREEN_H - 80}, 4, LIGHTGRAY); // Blade
        DrawLineEx({(float)SCREEN_W - 75, (float)SCREEN_H - 35}, {(float)SCREEN_W - 60, (float)SCREEN_H - 20}, 4, BROWN); // Hilt
    }
    DrawText("[Q] Switch", SCREEN_W - 95, SCREEN_H - 20, 14, WHITE);
}

void DrawSkillTreeUI(const GameWorld& world) {
    const Player& pl = world.player;
    const Profile& prof = world.profile;
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, {5, 10, 25, 240});
    DrawText("SKILL TREE", SCREEN_W / 2 - 80, 15, 28, GOLD);
    DrawText(TextFormat("Skill Points: %d  |  Stat Points: %d", pl.stats.unspentSkillPoints, pl.stats.unspentStatPoints), SCREEN_W / 2 - 180, 48, 16, SKYBLUE);

    // Discipline tabs at top
    for (int d = 0; d < DISCIPLINE_COUNT; d++) {
        float tabX = 30 + d * 208;
        bool selected = (world.selectedDiscipline == d);
        Color tabCol = selected ? DisciplineColor((Discipline)d) : Color{40, 40, 50, 255};
        DrawRectangle((int)tabX, 75, 198, 30, tabCol);
        DrawRectangleLinesEx({tabX, 75, 198, 30}, 1, DisciplineColor((Discipline)d));
        DrawText(DisciplineName((Discipline)d), (int)tabX + 10, 80, 18, selected ? BLACK : LIGHTGRAY);
    }

    // Selected discipline abilities
    int d = world.selectedDiscipline;
    int ids[ABILITIES_PER_DISCIPLINE];
    GetDisciplineAbilities((Discipline)d, ids);

    for (int a = 0; a < ABILITIES_PER_DISCIPLINE; a++) {
        const AbilityDef& ability = GetAbility(ids[a]);
        float x = 60, y = 120 + a * 80.0f;
        float w = SCREEN_W - 120, h = 72;
        Rectangle r = {x, y, w, h};

        bool unlocked = prof.unlockedAbilities[ids[a]];
        bool canUnlock = CanUnlockAbility(prof, pl, ids[a]);
        int eqAt = -1;
        for (int i = 0; i < MAX_ABILITY_SLOTS; i++)
            if (prof.equippedSlots[i].abilityId == ids[a]) eqAt = i;

        // Background
        Color bg = {30, 30, 40, 255};
        if (eqAt != -1) bg = {20, 30, 60, 255};
        else if (unlocked) bg = {35, 40, 55, 255};
        else if (canUnlock) bg = {40, 35, 30, 255};
        DrawRectangleRec(r, bg);

        // Border
        Color border = unlocked ? ability.color : (canUnlock ? ORANGE : Color{60, 60, 70, 255});
        if (eqAt != -1) border = GOLD;
        DrawRectangleLinesEx(r, 2, border);

        // Tier indicator
        Color tierColor = WHITE;
        switch (ability.tier) {
            case AbilityTier::BASIC: tierColor = LIGHTGRAY; break;
            case AbilityTier::INTERMEDIATE: tierColor = GREEN; break;
            case AbilityTier::ADVANCED: tierColor = {80, 140, 255, 255}; break;
            case AbilityTier::ULTIMATE: tierColor = GOLD; break;
        }
        DrawRectangle((int)x, (int)y, 6, (int)h, tierColor);

        // Name and description
        DrawText(ability.name, (int)x + 15, (int)y + 8, 18, WHITE);
        DrawText(ability.desc, (int)x + 15, (int)y + 30, 14, LIGHTGRAY);

        // Tier label
        DrawText(TierName(ability.tier), (int)(x + w - 140), (int)y + 8, 14, tierColor);

        // Stat requirement
        if (ability.primaryStatIndex >= 0) {
            const char* req = TextFormat("Req: %s %d", StatAbbrev(ability.primaryStatIndex), ability.primaryStatReq);
            bool met = pl.stats.GetByIndex(ability.primaryStatIndex) >= ability.primaryStatReq;
            DrawText(req, (int)(x + w - 140), (int)y + 28, 12, met ? GREEN : RED);
        }
        if (ability.secondaryStatIndex >= 0) {
            const char* req = TextFormat("+ %s %d", StatAbbrev(ability.secondaryStatIndex), ability.secondaryStatReq);
            bool met = pl.stats.GetByIndex(ability.secondaryStatIndex) >= ability.secondaryStatReq;
            DrawText(req, (int)(x + w - 140), (int)y + 42, 12, met ? GREEN : RED);
        }

        // Cost / status
        if (eqAt != -1) {
            DrawText(TextFormat("SLOT %d", eqAt + 1), (int)(x + w - 65), (int)y + 52, 14, GOLD);
        } else if (!unlocked) {
            DrawText(TextFormat("SP: %d", ability.skillPointCost), (int)(x + w - 55), (int)y + 52, 14, canUnlock ? ORANGE : GRAY);
        }

        // Cooldown info
        DrawText(TextFormat("CD: %.1fs", ability.cooldown), (int)x + 15, (int)y + 52, 12, {150, 150, 150, 255});
    }

    DrawText("[TAB] Close  |  LMB: Unlock/Equip  |  Click tabs to switch discipline", 60, SCREEN_H - 35, 16, GRAY);
}

void DrawHub(const GameWorld& world) {
    ClearBackground({5, 5, 15, 255});
    DrawText("GUILD HALL", SCREEN_W / 2 - 100, 60, 36, SKYBLUE);
    DrawText(TextFormat("Hunter Rank: %s", RankName((Rank)world.profile.hunterRankLevel)), SCREEN_W / 2 - 80, 110, 24, GOLD);
    DrawText(TextFormat("Level: %d  |  Credits: %d", world.profile.playerLevel, world.profile.money), SCREEN_W / 2 - 120, 145, 20, LIGHTGRAY);

    // Stats summary
    const Attributes& s = world.profile.permanentStats;
    DrawText(TextFormat("STR:%d AGI:%d VIT:%d INT:%d WIS:%d DEX:%d SPR:%d END:%d",
        s.STR, s.AGI, s.VIT, s.INT, s.WIS, s.DEX, s.SPR, s.END),
        SCREEN_W / 2 - 300, 180, 16, {150, 150, 180, 255});

    // Missions
    DrawText("Active Missions:", SCREEN_W / 2 - 100, 230, 20, SKYBLUE);
    for (int i = 0; i < MAX_MISSIONS; i++) {
        const Mission& m = world.profile.activeMissions[i];
        Color c = m.completed ? GREEN : WHITE;
        const char* typeStr = (m.type == 0) ? "Kill" : "Hunt Brutes";
        DrawText(TextFormat("%s %d/%d  [%dXP, %d$]", typeStr, m.currentKills, m.targetKills, m.rewardXp, m.rewardMoney),
            SCREEN_W / 2 - 150, 260 + i * 25, 16, c);
    }

    // Controls
    int cy = 420;
    DrawText("[ENTER] Enter Rift", SCREEN_W / 2 - 100, cy, 20, GREEN);
    DrawText("[TAB] Skill Tree", SCREEN_W / 2 - 100, cy + 30, 20, LIGHTGRAY);
    DrawText("[C] Status / Stats", SCREEN_W / 2 - 100, cy + 60, 20, LIGHTGRAY);
    DrawText("[I] Inventory", SCREEN_W / 2 - 100, cy + 90, 20, LIGHTGRAY);
    DrawText("[M] Market", SCREEN_W / 2 - 100, cy + 120, 20, LIGHTGRAY);
}

void DrawStatusWindow(GameWorld& world) {
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, {5, 15, 35, 245});
    Player& pl = world.player;
    Attributes& s = pl.stats;

    DrawText("HUNTER STATUS", SCREEN_W / 2 - 120, 40, 32, SKYBLUE);
    DrawText(TextFormat("Level: %d  |  XP: %d/%d", pl.level, pl.xp, pl.xpToNext), SCREEN_W / 2 - 140, 80, 18, LIGHTGRAY);
    DrawText(TextFormat("Stat Points: %d  |  Skill Points: %d", s.unspentStatPoints, s.unspentSkillPoints), SCREEN_W / 2 - 160, 105, 18, SKYBLUE);

    // Draw all 8 stats with + buttons
    const char* statDescriptions[] = {
        "Phys damage +8%/pt",
        "Move speed +12/pt, Atk speed +2%/pt",
        "Max HP +25/pt, Regen +0.3/pt",
        "Magic damage +8%/pt, CDR +1.5%/pt",
        "Heal power +10%/pt, Buff duration +3%/pt",
        "Crit chance +1.2%/pt, Crit dmg +2%/pt",
        "Summon power +10%/pt",
        "Damage reduction +1.2%/pt, Shield +15/pt"
    };

    for (int i = 0; i < STAT_COUNT; i++) {
        float y = 150 + i * 55.0f;
        int val = s.GetByIndex(i);
        DrawText(StatAbbrev(i), 250, (int)y, 22, DisciplineColor((Discipline)(i < 6 ? i : i - 2)));
        DrawText(TextFormat("%d", val), 330, (int)y, 22, GOLD);
        DrawText(statDescriptions[i], 420, (int)y + 4, 14, {150, 150, 160, 255});

        if (s.unspentStatPoints > 0) {
            Rectangle btn = {380, y - 2, 28, 28};
            if (CheckCollisionPointRec(GetMousePosition(), btn)) {
                DrawRectangleRec(btn, {80, 200, 80, 200});
            }
            DrawRectangleLinesEx(btn, 1, GREEN);
            DrawText("+", 388, (int)y + 2, 20, GREEN);
        }
    }

    // Derived stats on the right
    DrawText("--- Derived Stats ---", 800, 150, 18, GOLD);
    DrawText(TextFormat("Max HP: %.0f", pl.maxHp), 800, 180, 16, WHITE);
    DrawText(TextFormat("Phys Dmg: x%.2f", pl.physDamageMultiplier), 800, 200, 16, WHITE);
    DrawText(TextFormat("Magic Dmg: x%.2f", pl.magicDamageMultiplier), 800, 220, 16, WHITE);
    DrawText(TextFormat("Crit: %.1f%% (x%.2f)", pl.critChance * 100, pl.critMultiplier), 800, 240, 16, WHITE);
    DrawText(TextFormat("Heal Power: x%.2f", pl.healPower), 800, 260, 16, WHITE);
    DrawText(TextFormat("Summon Power: x%.2f", pl.summonPower), 800, 280, 16, WHITE);
    DrawText(TextFormat("Dmg Reduction: %.1f%%", pl.damageReduction * 100), 800, 300, 16, WHITE);
    DrawText(TextFormat("CDR: %.1f%%", pl.cooldownReduction * 100), 800, 320, 16, WHITE);
    DrawText(TextFormat("Speed: %.0f", pl.speed), 800, 340, 16, WHITE);

    DrawText("[C] Close  |  Click [+] to allocate stat points", 250, SCREEN_H - 40, 16, GRAY);
}

void DrawInventoryUI(GameWorld& world) {
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, {10, 10, 20, 240});
    DrawText("INVENTORY", SCREEN_W / 2 - 80, 20, 28, GOLD);
    DrawText(TextFormat("Credits: %d", world.profile.money), SCREEN_W - 220, 20, 18, GREEN);

    // Equipped slots (left side)
    DrawText("Equipped:", 30, 60, 20, SKYBLUE);
    for (int i = 0; i < EQUIP_SLOT_COUNT; i++) {
        float y = 90 + i * 70.0f;
        Rectangle r = {30, y, 280, 62};
        int idx = world.profile.equippedIndices[i];
        bool hasItem = (idx >= 0 && idx < MAX_INVENTORY && world.profile.inventory[idx].active);

        DrawRectangleRec(r, {25, 25, 35, 255});
        DrawRectangleLinesEx(r, 1, hasItem ? RarityColor(world.profile.inventory[idx].rarity) : DARKGRAY);
        DrawText(EquipSlotName((EquipSlot)i), 40, (int)y + 5, 14, GRAY);

        if (hasItem) {
            const Equipment& eq = world.profile.inventory[idx];
            DrawText(eq.name, 40, (int)y + 22, 16, RarityColor(eq.rarity));
            DrawText(TextFormat("Lv.%d %s", eq.itemLevel, RarityName(eq.rarity)), 40, (int)y + 42, 12, LIGHTGRAY);
        }
    }

    // Inventory grid (right side)
    DrawText("Backpack:", 350, 60, 20, SKYBLUE);
    int col = 0, row = 0;
    for (int i = 0; i < MAX_INVENTORY; i++) {
        if (!world.profile.inventory[i].active) continue;
        // Skip equipped items
        bool equipped = false;
        for (int s = 0; s < EQUIP_SLOT_COUNT; s++)
            if (world.profile.equippedIndices[s] == i) equipped = true;

        float x = 350 + col * 300;
        float y = 90 + row * 55.0f;
        if (y > SCREEN_H - 80) break;

        Rectangle r = {x, y, 290, 50};
        const Equipment& eq = world.profile.inventory[i];
        DrawRectangleRec(r, equipped ? Color{20, 30, 50, 255} : Color{25, 25, 35, 255});
        DrawRectangleLinesEx(r, 1, RarityColor(eq.rarity));
        DrawText(eq.name, (int)x + 8, (int)y + 5, 14, RarityColor(eq.rarity));
        DrawText(TextFormat("%s Lv.%d", EquipSlotName(eq.slot), eq.itemLevel), (int)x + 8, (int)y + 24, 12, LIGHTGRAY);
        if (equipped) DrawText("E", (int)x + 270, (int)y + 5, 14, GOLD);
        DrawText(TextFormat("%d$", eq.sellPrice), (int)x + 240, (int)y + 30, 12, GREEN);

        col++;
        if (col >= 3) { col = 0; row++; }
        else row += (col == 1) ? 0 : 0; // stay same row
        if (col == 0) row++;
    }

    DrawText("[I] Close  |  LMB: Equip/Unequip  |  RMB: Sell", 30, SCREEN_H - 35, 16, GRAY);
}

void DrawRiftSelectUI(const GameWorld& world) {
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, {5, 5, 15, 240});
    DrawText("SELECT RIFT DIFFICULTY", SCREEN_W / 2 - 180, 60, 32, GOLD);
    DrawText(TextFormat("Hunter Rank: %s", RankName((Rank)world.profile.hunterRankLevel)), SCREEN_W / 2 - 80, 100, 20, SKYBLUE);

    for (int i = 0; i <= 6; i++) {
        // Calculate position - center the row of 7 items
        float startX = (SCREEN_W - (7 * 160)) / 2 + 10;
        float x = startX + i * 160.0f;
        float y = SCREEN_H / 2 - 90;
        
        bool available = (i <= world.profile.hunterRankLevel);
        bool selected = (world.selectedRiftRank == i);

        Rectangle r = {x, y, 140, 180};
        
        // Background
        Color bg = available ? (selected ? Color{30, 40, 60, 255} : Color{20, 20, 30, 255}) : Color{10, 10, 10, 255};
        DrawRectangleRec(r, bg);
        
        // Border
        Color border = available ? RankColor((Rank)i) : DARKGRAY;
        if (selected && available) border = WHITE;
        DrawRectangleLinesEx(r, 2, border);

        // Rank Name
        DrawText(TextFormat("Rank %s", RankName((Rank)i)), (int)x + 35, (int)y + 15, 24, available ? RankColor((Rank)i) : DARKGRAY);
        
        if (available) {
            DrawText(TextFormat("Enemies: %d", 8 + i * 2), (int)x + 10, (int)y + 55, 14, LIGHTGRAY);
            DrawText(TextFormat("Rooms: %d", 3), (int)x + 10, (int)y + 75, 14, LIGHTGRAY);
            
            if (i >= 2) DrawText("+ Elites", (int)x + 10, (int)y + 95, 14, ORANGE);
            if (i >= 5) DrawText("+ Boss", (int)x + 10, (int)y + 115, 14, RED);
            
            if (selected) {
                 DrawText("START", (int)x + 40, (int)y + 145, 18, GREEN);
                 DrawRectangleLinesEx(r, 3, GREEN); // Highlight selection
            }
        } else {
            // Locked Icon (Simple drawing)
            DrawRectangle((int)x + 55, (int)y + 70, 30, 25, DARKGRAY); // Body
            DrawRectangleLines((int)x + 55, (int)y + 70, 30, 25, GRAY);
            DrawCircleLines((int)x + 70, (int)y + 70, 10, GRAY); // Shackle (approx)
            DrawText("LOCKED", (int)x + 35, (int)y + 110, 18, RED);
            DrawText("Req. Promotion", (int)x + 15, (int)y + 135, 14, DARKGRAY);
        }
    }

    DrawText("[ESC] Back  |  LEFT/RIGHT to select  |  [ENTER] to enter", SCREEN_W / 2 - 220, SCREEN_H - 50, 20, LIGHTGRAY);
}

void DrawMinimap(const Rift& rift, Vector2 playerPos) {
    int mapSize = 200;
    int xStart = SCREEN_W - mapSize - 20;
    int yStart = 20;
    
    // Background
    DrawRectangle(xStart, yStart, mapSize, mapSize, {0, 0, 0, 150});
    DrawRectangleLines(xStart, yStart, mapSize, mapSize, DARKGRAY);
    
    // Draw explored tiles (each tile is 2x2 pixels)
    // 100 tiles * 2px = 200px size
    for (int x = 0; x < DUNGEON_WIDTH; x++) {
        for (int y = 0; y < DUNGEON_HEIGHT; y++) {
            if (rift.exploredMap[x][y]) {
                int px = xStart + x * 2;
                int py = yStart + y * 2;
                if (rift.tileMap[x][y] == 1) {
                    DrawRectangle(px, py, 2, 2, LIGHTGRAY);
                } else {
                    DrawRectangle(px, py, 2, 2, {60, 60, 60, 255}); // Floor
                }
            }
        }
    }
    
    // Draw player dot
    int pX = (int)(playerPos.x / TILE_SIZE);
    int pY = (int)(playerPos.y / TILE_SIZE);
    DrawRectangle(xStart + pX * 2 - 1, yStart + pY * 2 - 1, 4, 4, GREEN);
}

void DrawMarketUI(const GameWorld& world) {
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, {10, 20, 10, 240});
    DrawText("BLACK MARKET", SCREEN_W / 2 - 100, 60, 32, GOLD);
    DrawText(TextFormat("Credits: %d", world.profile.money), SCREEN_W / 2 - 60, 100, 24, GREEN);

    // Stat respec
    DrawRectangleLinesEx({100, 160, 300, 60}, 2, world.profile.money >= 2000 ? GOLD : RED);
    DrawText("Stat Respec Potion", 115, 170, 20, WHITE);
    DrawText("Reset all stat points - 2000$", 115, 195, 14, world.profile.money >= 2000 ? GREEN : RED);

    // Stat boosts (8 stats now)
    DrawText("Permanent +3 Stat Boosts (500$ each):", 100, 240, 18, LIGHTGRAY);
    for (int i = 0; i < STAT_COUNT; i++) {
        float x = 100 + (i % 4) * 280;
        float y = 275 + (i / 4) * 80;
        Rectangle r = {x, y, 260, 65};
        DrawRectangleLinesEx(r, 2, world.profile.money >= 500 ? GOLD : RED);
        DrawText(TextFormat("%s Boost (+3)", StatName(i)), (int)x + 15, (int)y + 10, 18, WHITE);
        DrawText(TextFormat("Current: %d", world.profile.permanentStats.GetByIndex(i)), (int)x + 15, (int)y + 35, 14, LIGHTGRAY);
    }

    DrawText("[M] Close", SCREEN_W / 2 - 40, SCREEN_H - 40, 18, GRAY);
}

void DrawRoomClear(const Rift& rift) {
    DrawText("SECTOR CLEAR", SCREEN_W / 2 - 120, SCREEN_H / 2, 40, GREEN);
}

void DrawRiftComplete(const Rift& rift) {
    DrawText("RIFT CONQUERED", SCREEN_W / 2 - 180, SCREEN_H / 2 - 30, 50, GOLD);
    DrawText(TextFormat("Rank %s Complete!", RankName(rift.rank)), SCREEN_W / 2 - 100, SCREEN_H / 2 + 30, 24, SKYBLUE);
    DrawText("[ENTER] Return to Hub", SCREEN_W / 2 - 120, SCREEN_H / 2 + 70, 20, LIGHTGRAY);
}

void DrawGameOver(int killCount) {
    DrawText("HUNTER DEFEATED", SCREEN_W / 2 - 180, SCREEN_H / 2 - 20, 50, RED);
    DrawText(TextFormat("Kills: %d", killCount), SCREEN_W / 2 - 40, SCREEN_H / 2 + 40, 20, LIGHTGRAY);
    DrawText("[ENTER] Return to Hub", SCREEN_W / 2 - 120, SCREEN_H / 2 + 70, 20, LIGHTGRAY);
}

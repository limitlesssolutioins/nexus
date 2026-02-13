#include "progression.h"
#include "persistence.h"

// Helper to build AbilityDef compactly
static AbilityDef MakeAbility(int id, const char* name, const char* desc,
    Discipline disc, AbilityTier tier, int spCost, float cd, Color col,
    int priStat, int priReq, int secStat, int secReq,
    int scaleStat, float baseVal, float scaleFactor) {
    AbilityDef a{};
    a.id = id;
    strncpy(a.name, name, 31);
    strncpy(a.desc, desc, 95);
    a.discipline = disc; a.tier = tier; a.skillPointCost = spCost;
    a.cooldown = cd; a.color = col;
    a.primaryStatIndex = priStat; a.primaryStatReq = priReq;
    a.secondaryStatIndex = secStat; a.secondaryStatReq = secReq;
    a.scalingStatIndex = scaleStat; a.baseValue = baseVal; a.scalingFactor = scaleFactor;
    return a;
}

// Stat indices: 0=STR, 1=AGI, 2=VIT, 3=INT, 4=WIS, 5=DEX, 6=SPR, 7=END
const AbilityDef ALL_ABILITIES[TOTAL_ABILITIES] = {
    // ===== WARRIOR (0-6) — Primary: STR(0), Secondary: END(7) =====
    MakeAbility( 0, "Cleave",           "Wide 120deg melee arc",           Discipline::WARRIOR, AbilityTier::BASIC,        1, 1.5f, {220,80,60,255},   -1, 0, -1, 0,  0, 40, 3.0f),
    MakeAbility( 1, "War Cry",          "+20% damage for 8s, draw aggro",  Discipline::WARRIOR, AbilityTier::BASIC,        1, 12.0f,{200,60,40,255},   -1, 0, -1, 0,  0, 0, 0),
    MakeAbility( 2, "Whirlwind",        "Spin attack 3 hits over 1.5s",    Discipline::WARRIOR, AbilityTier::INTERMEDIATE, 2, 6.0f, {230,90,50,255},    0,20, -1, 0,  0, 50, 3.5f),
    MakeAbility( 3, "Iron Skin",        "Reduce damage 40% for 5s",       Discipline::WARRIOR, AbilityTier::INTERMEDIATE, 2, 15.0f,{180,160,120,255},  7,20, -1, 0,  7, 0, 0),
    MakeAbility( 4, "Execution Strike", "Massive hit, +100% on low HP",   Discipline::WARRIOR, AbilityTier::ADVANCED,     3, 10.0f,{255,50,30,255},    0,35, -1, 0,  0, 80, 5.0f),
    MakeAbility( 5, "Unbreakable",      "Immune knockback +30% END 8s",   Discipline::WARRIOR, AbilityTier::ADVANCED,     3, 20.0f,{160,140,100,255},  7,35, -1, 0,  7, 0, 0),
    MakeAbility( 6, "Berserker Rage",   "2x STR, +50% speed, -3%HP/s",   Discipline::WARRIOR, AbilityTier::ULTIMATE,     5, 45.0f,{255,30,10,255},    0,50,  7,25,  0, 0, 0),

    // ===== MAGE (7-13) — Primary: INT(3), Secondary: WIS(4) =====
    MakeAbility( 7, "Arcane Bolt",      "Fast magic projectile",          Discipline::MAGE, AbilityTier::BASIC,        1, 0.8f, {100,150,255,255},  -1, 0, -1, 0,  3, 25, 2.5f),
    MakeAbility( 8, "Frost Nova",       "AoE slow 50% for 4s",           Discipline::MAGE, AbilityTier::BASIC,        1, 8.0f, {150,220,255,255},  -1, 0, -1, 0,  3, 20, 1.5f),
    MakeAbility( 9, "Fireball",         "Explosive projectile + burn",    Discipline::MAGE, AbilityTier::INTERMEDIATE, 2, 3.0f, {255,120,30,255},    3,20, -1, 0,  3, 60, 4.0f),
    MakeAbility(10, "Lightning Chain",  "Chain hits to 3 nearby enemies", Discipline::MAGE, AbilityTier::INTERMEDIATE, 2, 5.0f, {255,255,100,255},   3,20, -1, 0,  3, 45, 3.0f),
    MakeAbility(11, "Meteor",           "Delayed massive AoE impact",     Discipline::MAGE, AbilityTier::ADVANCED,     3, 15.0f,{255,80,20,255},     3,35, -1, 0,  3, 120, 6.0f),
    MakeAbility(12, "Mana Shield",      "Convert damage to CD increase",  Discipline::MAGE, AbilityTier::ADVANCED,     3, 20.0f,{120,180,255,255},   4,35, -1, 0,  4, 0, 0),
    MakeAbility(13, "Arcane Singularity","Black hole pulls+damages 6s",   Discipline::MAGE, AbilityTier::ULTIMATE,     5, 50.0f,{80,50,220,255},     3,50,  4,25,  3, 100, 8.0f),

    // ===== SORCERER (14-20) — Primary: INT(3), Secondary: SPR(6) =====
    MakeAbility(14, "Hex Bolt",         "Projectile -15% enemy dmg 5s",   Discipline::SORCERER, AbilityTier::BASIC,        1, 2.0f, {180,50,200,255},  -1, 0, -1, 0,  3, 20, 2.0f),
    MakeAbility(15, "Drain Touch",      "Short range dmg + heal 30%",     Discipline::SORCERER, AbilityTier::BASIC,        1, 4.0f, {160,30,180,255},  -1, 0, -1, 0,  3, 30, 2.5f),
    MakeAbility(16, "Curse of Weakness","-25% enemy speed+dmg 8s AoE",   Discipline::SORCERER, AbilityTier::INTERMEDIATE, 2, 10.0f,{140,20,160,255},   3,20, -1, 0,  3, 0, 0),
    MakeAbility(17, "Soul Burn",        "DOT 5% maxHP/s for 6s",         Discipline::SORCERER, AbilityTier::INTERMEDIATE, 2, 8.0f, {200,60,220,255},   3,20, -1, 0,  3, 15, 2.0f),
    MakeAbility(18, "Void Prison",      "Immobilize 3s, dmg on release", Discipline::SORCERER, AbilityTier::ADVANCED,     3, 18.0f,{120,10,180,255},   3,35, -1, 0,  3, 50, 4.0f),
    MakeAbility(19, "Life Siphon",      "Channel drain+heal over time",   Discipline::SORCERER, AbilityTier::ADVANCED,     3, 15.0f,{200,80,255,255},   6,35, -1, 0,  3, 20, 3.0f),
    MakeAbility(20, "Domain Expansion", "All enemies 2x dmg, no heal 10s",Discipline::SORCERER, AbilityTier::ULTIMATE,     5, 60.0f,{100,0,150,255},    3,50,  6,25,  3, 0, 0),

    // ===== HUNTER (21-27) — Primary: AGI(1), Secondary: DEX(5) =====
    MakeAbility(21, "Quick Shot",       "Fast ranged attack, high crit",  Discipline::HUNTER, AbilityTier::BASIC,        1, 0.5f, {50,200,100,255},   -1, 0, -1, 0,  5, 18, 2.0f),
    MakeAbility(22, "Dash",             "Quick move + brief i-frames",    Discipline::HUNTER, AbilityTier::BASIC,        1, 2.0f, {80,255,130,255},   -1, 0, -1, 0,  1, 0, 0),
    MakeAbility(23, "Fan of Knives",    "8 projectile spread, can crit",  Discipline::HUNTER, AbilityTier::INTERMEDIATE, 2, 5.0f, {60,220,110,255},    1,20, -1, 0,  5, 15, 1.8f),
    MakeAbility(24, "Trap",             "Place trap, root 3s + damage",   Discipline::HUNTER, AbilityTier::INTERMEDIATE, 2, 8.0f, {90,180,70,255},     5,20, -1, 0,  5, 35, 2.5f),
    MakeAbility(25, "Assassinate",      "Teleport behind, 3x stealth dmg",Discipline::HUNTER, AbilityTier::ADVANCED,     3, 12.0f,{30,160,60,255},     1,35, -1, 0,  5, 60, 5.0f),
    MakeAbility(26, "Evasion Mastery",  "80% dodge 4s, counter on dodge", Discipline::HUNTER, AbilityTier::ADVANCED,     3, 18.0f,{70,255,160,255},    5,35, -1, 0,  5, 0, 0),
    MakeAbility(27, "Phantom Rush",     "10 rapid crits 2s, invulnerable",Discipline::HUNTER, AbilityTier::ULTIMATE,     5, 45.0f,{20,255,80,255},     1,50,  5,25,  5, 40, 6.0f),

    // ===== SUMMONER (28-34) — Primary: SPR(6), Secondary: INT(3) =====
    MakeAbility(28, "Summon Wisp",      "Auto-attack wisp companion",     Discipline::SUMMONER, AbilityTier::BASIC,        1, 6.0f, {200,200,255,255},  -1, 0, -1, 0,  6, 15, 2.0f),
    MakeAbility(29, "Spirit Link",      "Link enemy, share 30% dmg",     Discipline::SUMMONER, AbilityTier::BASIC,        1, 10.0f,{180,180,240,255},  -1, 0, -1, 0,  6, 0, 0),
    MakeAbility(30, "Summon Guardian",  "Tank minion that taunts",        Discipline::SUMMONER, AbilityTier::INTERMEDIATE, 2, 15.0f,{150,150,220,255},   6,20, -1, 0,  6, 80, 6.0f),
    MakeAbility(31, "Empower Summons",  "All summons +50% dmg 10s",      Discipline::SUMMONER, AbilityTier::INTERMEDIATE, 2, 12.0f,{220,220,255,255},   6,20, -1, 0,  6, 0, 0),
    MakeAbility(32, "Summon Shadow",    "Clone mirrors attacks at 60%",   Discipline::SUMMONER, AbilityTier::ADVANCED,     3, 25.0f,{100,100,200,255},   6,35, -1, 0,  6, 50, 5.0f),
    MakeAbility(33, "Soul Storm",       "Sacrifice summons, AoE explode", Discipline::SUMMONER, AbilityTier::ADVANCED,     3, 20.0f,{180,80,255,255},    3,35, -1, 0,  6, 60, 5.0f),
    MakeAbility(34, "Arise",            "Resurrect dead enemies as allies",Discipline::SUMMONER, AbilityTier::ULTIMATE,     5, 90.0f,{80,60,180,255},    6,50,  3,25,  6, 0, 0),

    // ===== HEALER (35-41) — Primary: WIS(4), Secondary: SPR(6) =====
    MakeAbility(35, "Mend",             "Instant WIS-scaled heal",        Discipline::HEALER, AbilityTier::BASIC,        1, 3.0f, {100,255,150,255},  -1, 0, -1, 0,  4, 30, 3.0f),
    MakeAbility(36, "Purify",           "Cleanse debuffs + small heal",   Discipline::HEALER, AbilityTier::BASIC,        1, 8.0f, {200,255,200,255},  -1, 0, -1, 0,  4, 15, 1.5f),
    MakeAbility(37, "Regen Aura",       "HoT in radius around player 10s",Discipline::HEALER, AbilityTier::INTERMEDIATE, 2, 15.0f,{120,255,180,255},   4,20, -1, 0,  4, 5, 1.0f),
    MakeAbility(38, "Holy Shield",      "Absorb shield WIS-scaled 8s",   Discipline::HEALER, AbilityTier::INTERMEDIATE, 2, 12.0f,{255,255,100,255},   4,20, -1, 0,  4, 40, 4.0f),
    MakeAbility(39, "Resurrection",     "Auto-revive on death (once/rift)",Discipline::HEALER, AbilityTier::ADVANCED,     3, 0.0f, {255,220,60,255},    4,35, -1, 0,  4, 0, 0),
    MakeAbility(40, "Sanctuary",        "Zone: enemies -50% dmg 8s",     Discipline::HEALER, AbilityTier::ADVANCED,     3, 25.0f,{180,255,220,255},   6,35, -1, 0,  4, 0, 0),
    MakeAbility(41, "Divine Judgment",  "Massive heal+dmg, scales missHP",Discipline::HEALER, AbilityTier::ULTIMATE,     5, 50.0f,{255,240,100,255},   4,50,  6,25,  4, 80, 8.0f),
};

const AbilityDef& GetAbility(int id) {
    if (id < 0 || id >= TOTAL_ABILITIES) return ALL_ABILITIES[0];
    return ALL_ABILITIES[id];
}

void GetDisciplineAbilities(Discipline d, int outIds[ABILITIES_PER_DISCIPLINE]) {
    int base = (int)d * ABILITIES_PER_DISCIPLINE;
    for (int i = 0; i < ABILITIES_PER_DISCIPLINE; i++) outIds[i] = base + i;
}

// --- Stat Formulas ---
void RecalculateStats(Player& p, const Profile& profile) {
    // Start from base permanent stats
    Attributes total = profile.permanentStats;
    float def = 0;
    // Add equipment bonuses
    for (int i = 0; i < EQUIP_SLOT_COUNT; i++) {
        int idx = profile.equippedIndices[i];
        if (idx >= 0 && idx < MAX_INVENTORY && profile.inventory[idx].active) {
            const Attributes& bonus = profile.inventory[idx].bonusStats;
            total.STR += bonus.STR; total.AGI += bonus.AGI;
            total.VIT += bonus.VIT; total.INT += bonus.INT;
            total.WIS += bonus.WIS; total.DEX += bonus.DEX;
            total.SPR += bonus.SPR; total.END += bonus.END;
            def += profile.inventory[idx].defense;
        }
    }
    total.unspentStatPoints = profile.permanentStats.unspentStatPoints;
    total.unspentSkillPoints = profile.permanentStats.unspentSkillPoints;
    p.stats = total;
    p.flatDefense = def;

    // Derived stats
    p.baseMaxHp = 100.0f + total.VIT * 25.0f;
    p.maxHp = p.baseMaxHp;
    p.speed = 280.0f + total.AGI * 12.0f;
    p.attackSpeedMultiplier = 1.0f + total.AGI * 0.02f;
    p.physDamageMultiplier = 1.0f + total.STR * 0.08f;
    p.magicDamageMultiplier = 1.0f + total.INT * 0.08f;
    p.healPower = 1.0f + total.WIS * 0.10f;
    p.cooldownReduction = fminf(total.INT * 0.015f, 0.50f);
    p.critChance = 0.05f + total.DEX * 0.012f;
    if (p.critChance > 0.60f) p.critChance = 0.60f;
    p.critMultiplier = 1.5f + total.DEX * 0.02f;
    p.summonPower = 1.0f + total.SPR * 0.10f;
    p.damageReduction = fminf(total.END * 0.012f, 0.60f);
}

Player MakePlayer(const Profile& profile) {
    Player p{};
    p.position = {350.0f, 350.0f}; // Start in safe zone (5,5) * 64
    p.radius = 20.0f;
    p.fireTimer = 0;
    p.baseFireRate = 0.15f;
    p.invTime = 0.5f;
    p.dmgCooldown = 0;
    // Level (synced from profile)
    p.level = profile.playerLevel;
    p.xp = profile.playerXp;
    p.xpToNext = XPForPlayerLevel(p.level);
    // Abilities
    for (int i = 0; i < TOTAL_ABILITIES; i++) p.unlockedAbilities[i] = profile.unlockedAbilities[i];
    for (int i = 0; i < MAX_ABILITY_SLOTS; i++) {
        p.slots[i] = profile.equippedSlots[i];
    }
    // Calculate all stats from profile + equipment
    RecalculateStats(p, profile);
    // Apply cooldown reduction to equipped abilities
    for (int i = 0; i < MAX_ABILITY_SLOTS; i++) {
        if (p.slots[i].abilityId >= 0) {
            p.slots[i].cooldownMax = GetAbility(p.slots[i].abilityId).cooldown * (1.0f - p.cooldownReduction);
            p.slots[i].cooldownTimer = 0;
        }
    }
    p.hp = p.maxHp;
    p.killCount = 0;
    p.bonusDamage = 0;
    return p;
}

bool AwardXP(Player& p, Profile& profile, int amount) {
    p.xp += amount;
    bool leveled = false;
    while (p.xp >= p.xpToNext && p.level < MAX_PLAYER_LEVEL) {
        p.xp -= p.xpToNext;
        p.level++;
        p.xpToNext = XPForPlayerLevel(p.level);
        p.stats.unspentStatPoints += STAT_POINTS_PER_LEVEL;
        p.stats.unspentSkillPoints += SKILL_POINTS_PER_LEVEL;
        p.hp = p.maxHp; // Full heal on level up
        leveled = true;
    }
    // Sync to profile
    profile.playerLevel = p.level;
    profile.playerXp = p.xp;
    profile.playerXpNext = p.xpToNext;
    profile.permanentStats.unspentStatPoints = p.stats.unspentStatPoints;
    profile.permanentStats.unspentSkillPoints = p.stats.unspentSkillPoints;
    return leveled;
}

bool CanUnlockAbility(const Profile& profile, const Player& p, int abilityId) {
    if (abilityId < 0 || abilityId >= TOTAL_ABILITIES) return false;
    if (profile.unlockedAbilities[abilityId]) return false;
    const AbilityDef& a = ALL_ABILITIES[abilityId];
    // Check skill points
    if (p.stats.unspentSkillPoints < a.skillPointCost) return false;
    // Check primary stat gate
    if (a.primaryStatIndex >= 0 && p.stats.GetByIndex(a.primaryStatIndex) < a.primaryStatReq) return false;
    // Check secondary stat gate
    if (a.secondaryStatIndex >= 0 && p.stats.GetByIndex(a.secondaryStatIndex) < a.secondaryStatReq) return false;
    return true;
}

bool TryUnlockAbility(Profile& profile, Player& p, int abilityId) {
    if (!CanUnlockAbility(profile, p, abilityId)) return false;
    const AbilityDef& a = ALL_ABILITIES[abilityId];
    p.stats.unspentSkillPoints -= a.skillPointCost;
    profile.permanentStats.unspentSkillPoints = p.stats.unspentSkillPoints;
    profile.unlockedAbilities[abilityId] = true;
    p.unlockedAbilities[abilityId] = true;
    return true;
}

bool EquipAbility(Profile& profile, Player& p, int abilityId, int slot) {
    if (slot < 0 || slot >= MAX_ABILITY_SLOTS) return false;
    if (!profile.unlockedAbilities[abilityId]) return false;
    // Unequip from other slots if already equipped
    for (int i = 0; i < MAX_ABILITY_SLOTS; i++) {
        if (profile.equippedSlots[i].abilityId == abilityId) {
            profile.equippedSlots[i] = {-1, 0, 0};
            p.slots[i] = {-1, 0, 0};
        }
    }
    const AbilityDef& a = ALL_ABILITIES[abilityId];
    float cd = a.cooldown * (1.0f - p.cooldownReduction);
    AbilitySlot s = {abilityId, 0, cd};
    p.slots[slot] = s;
    profile.equippedSlots[slot] = s;
    return true;
}

void UnequipSlot(Profile& profile, Player& p, int slot) {
    if (slot >= 0 && slot < MAX_ABILITY_SLOTS) {
        p.slots[slot] = {-1, 0, 0};
        profile.equippedSlots[slot] = {-1, 0, 0};
    }
}

// --- Equipment ---
int FindFreeInventorySlot(const Profile& profile) {
    for (int i = 0; i < MAX_INVENTORY; i++) {
        if (!profile.inventory[i].active) return i;
    }
    return -1;
}

bool EquipItem(Profile& profile, Player& p, int inventoryIndex) {
    if (inventoryIndex < 0 || inventoryIndex >= MAX_INVENTORY) return false;
    if (!profile.inventory[inventoryIndex].active) return false;
    Equipment& item = profile.inventory[inventoryIndex];
    int slotIdx = (int)item.slot;
    // Unequip current item in that slot
    if (profile.equippedIndices[slotIdx] >= 0) {
        // Just unlink, item stays in inventory
        profile.equippedIndices[slotIdx] = -1;
    }
    profile.equippedIndices[slotIdx] = inventoryIndex;
    RecalculateStats(p, profile);
    p.hp = fminf(p.hp, p.maxHp);
    return true;
}

void UnequipItem(Profile& profile, Player& p, EquipSlot slot) {
    int slotIdx = (int)slot;
    if (slotIdx >= 0 && slotIdx < EQUIP_SLOT_COUNT) {
        profile.equippedIndices[slotIdx] = -1;
        RecalculateStats(p, profile);
        p.hp = fminf(p.hp, p.maxHp);
    }
}

// --- Passive Updates ---
void UpdatePassives(Player& p, float dt) {
    // Damage cooldown
    if (p.dmgCooldown > 0) p.dmgCooldown -= dt;
    // Base HP regen (VIT-based)
    float baseRegen = 0.5f + (p.stats.VIT * 0.3f);
    p.hp = fminf(p.hp + baseRegen * dt, p.maxHp);
    // Buff timers
    // [0] Active regen buff
    if (p.buffTimers[0] > 0) {
        p.buffTimers[0] -= dt;
        p.regenTimer -= dt;
        if (p.regenTimer <= 0) {
            p.regenTimer = 1.0f;
            p.hp = fminf(p.hp + p.regenAmount * p.healPower, p.maxHp);
        }
    }
    // [1] Shield
    if (p.buffTimers[1] > 0) { p.buffTimers[1] -= dt; }
    else { p.shieldHp = 0; }
    // [2] Lifesteal
    if (p.buffTimers[2] > 0) { p.buffTimers[2] -= dt; }
    else { p.lifestealRatio = 0; }
    // [3] Damage reduction (non-passive, from abilities)
    if (p.buffTimers[3] > 0) { p.buffTimers[3] -= dt; }
    // [4] War Cry damage buff
    if (p.buffTimers[4] > 0) { p.buffTimers[4] -= dt; }
    else { p.bonusDamage = 0; }
    // [5] Iron Skin
    if (p.buffTimers[5] > 0) { p.buffTimers[5] -= dt; }
    // [6] Evasion
    if (p.buffTimers[6] > 0) { p.buffTimers[6] -= dt; }
    // [7] Reserved
    if (p.buffTimers[7] > 0) { p.buffTimers[7] -= dt; }
}

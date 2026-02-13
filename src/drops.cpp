#include "drops.h"
#include "progression.h"
#include <cstring>
#include <cstdio>

static const char* WEAPON_NAMES[] = {"Sword", "Staff", "Bow", "Dagger", "Mace", "Orb"};
static const char* HELMET_NAMES[] = {"Cap", "Helm", "Hood", "Crown", "Visor"};
static const char* ARMOR_NAMES[] = {"Vest", "Plate", "Robe", "Mail", "Tunic"};
static const char* BOOTS_NAMES[] = {"Boots", "Greaves", "Sandals", "Treads", "Sabatons"};
static const char* ACC_NAMES[] = {"Ring", "Amulet", "Charm", "Talisman", "Pendant"};
static const char* RARITY_PREFIX[] = {"", "Fine", "Superior", "Mystic", "Legendary"};

// Rarity stat ranges
static const int RARITY_MIN_STATS[] = {1, 2, 3, 4, 5};
static const int RARITY_MAX_STATS[] = {2, 3, 4, 6, 8};
static const int RARITY_MIN_BONUS[] = {1, 2, 4, 6, 10};
static const int RARITY_MAX_BONUS[] = {3, 5, 8, 12, 18};

Equipment GenerateEquipmentDrop(int riftRank, int enemyLevel, EnemyType sourceType) {
    Equipment eq{};
    eq.active = true;
    eq.itemLevel = enemyLevel + GetRandomValue(-1, 2);
    if (eq.itemLevel < 1) eq.itemLevel = 1;

    float roll = (float)GetRandomValue(0, 1000) / 10.0f;
    float legendaryThresh = 0.5f + riftRank * 0.5f;
    float epicThresh = legendaryThresh + 4.0f + riftRank * 1.0f;
    float rareThresh = epicThresh + 12.0f + riftRank * 1.5f;
    float uncommonThresh = rareThresh + 25.0f;

    if (sourceType == EnemyType::ELITE) { legendaryThresh *= 2; epicThresh *= 1.5f; }
    if (sourceType == EnemyType::BOSS) { legendaryThresh *= 4; epicThresh *= 2.0f; }

    if (roll < legendaryThresh) eq.rarity = Rarity::LEGENDARY;
    else if (roll < epicThresh) eq.rarity = Rarity::EPIC;
    else if (roll < rareThresh) eq.rarity = Rarity::RARE;
    else if (roll < uncommonThresh) eq.rarity = Rarity::UNCOMMON;
    else eq.rarity = Rarity::COMMON;

    int slotRoll = GetRandomValue(0, 100);
    if (slotRoll < 30) eq.slot = EquipSlot::WEAPON;
    else if (slotRoll < 50) eq.slot = EquipSlot::HELMET;
    else if (slotRoll < 70) eq.slot = EquipSlot::ARMOR;
    else if (slotRoll < 85) eq.slot = EquipSlot::BOOTS;
    else eq.slot = EquipSlot::ACCESSORY_1;

    if (eq.slot == EquipSlot::WEAPON) {
        eq.weaponType = (WeaponType)GetRandomValue(0, 5);
        if (sourceType == EnemyType::BRUTE) eq.weaponType = (GetRandomValue(0,100)<60)? WeaponType::MACE : WeaponType::SWORD;
        else if (sourceType == EnemyType::SPITTER) eq.weaponType = (GetRandomValue(0,100)<60)? WeaponType::STAFF : WeaponType::ORB;
    } else eq.weaponType = WeaponType::NONE;

    float minQ = (int)eq.rarity * 0.1f; 
    eq.quality = minQ + ((float)GetRandomValue(0, 100) / 100.0f) * (1.0f - minQ);
    float qualityMult = 0.8f + (eq.quality * 0.4f);
    
    float rankMult = 1.0f + (int)eq.rarity * 0.4f;

    if (eq.slot == EquipSlot::WEAPON) {
        float baseAtk = 10.0f + (eq.itemLevel * 2.0f);
        if (eq.weaponType == WeaponType::DAGGER) baseAtk *= 0.7f;
        if (eq.weaponType == WeaponType::MACE) baseAtk *= 1.4f;
        eq.attack = baseAtk * rankMult * qualityMult;
    } else {
        float baseDef = 5.0f + (eq.itemLevel * 1.5f);
        if (eq.slot == EquipSlot::ARMOR) baseDef *= 2.0f;
        eq.defense = baseDef * rankMult * qualityMult;
    }

    int rarIdx = (int)eq.rarity;
    int numStats = GetRandomValue(RARITY_MIN_STATS[rarIdx], RARITY_MAX_STATS[rarIdx]);
    for (int i = 0; i < numStats; i++) {
        eq.bonusStats.AddByIndex(GetRandomValue(0, 7), (int)(GetRandomValue(RARITY_MIN_BONUS[rarIdx], RARITY_MAX_BONUS[rarIdx]) * (1.0f + eq.itemLevel * 0.05f)));
    }

    const char* qp = (eq.quality < 0.2f)?"Damaged ":(eq.quality < 0.4f)?"Rusty ":(eq.quality > 0.8f)?"Fine ":"";
    const char* bn = (eq.slot == EquipSlot::WEAPON)? WEAPON_NAMES[(int)eq.weaponType] : ARMOR_NAMES[0];
    snprintf(eq.name, 63, "%s%s %s", qp, RARITY_PREFIX[rarIdx], bn);
    eq.sellPrice = (int)((rarIdx + 1) * 50 * qualityMult);

    return eq;
}

void TrySpawnEquipmentDrop(std::vector<EquipmentDrop>& drops, Vector2 pos, int riftRank, int enemyLevel, EnemyType sourceType) {
    int chance = 5; // Low chance for real gear
    if (sourceType == EnemyType::ELITE) chance = 50;
    if (sourceType == EnemyType::BOSS) chance = 100;

    if (GetRandomValue(0, 99) < chance) {
        EquipmentDrop d{};
        d.position = Vector2Add(pos, {(float)GetRandomValue(-10,10),(float)GetRandomValue(-10,10)});
        d.item = GenerateEquipmentDrop(riftRank, enemyLevel, sourceType);
        d.active = true;
        drops.push_back(d);
    }
}

void SpawnMaterialDrop(std::vector<MaterialDrop>& drops, Vector2 pos, int type, int amount) {
    for(int i=0; i<amount; i++) {
        MaterialDrop d{};
        d.position = Vector2Add(pos, {(float)GetRandomValue(-20,20),(float)GetRandomValue(-20,20)});
        d.type = type;
        d.amount = 1;
        d.lifetime = 25.0f;
        d.active = true;
        drops.push_back(d);
    }
}

void UpdateDrops(std::vector<EquipmentDrop>& drops, Player& p, Profile& profile, float dt) {
    for (auto& d : drops) {
        if (!d.active) continue;
        d.lifetime -= dt;
        if (Vector2Distance(p.position, d.position) < p.radius + 20) {
            int slot = FindFreeInventorySlot(profile);
            if (slot >= 0) { profile.inventory[slot] = d.item; d.active = false; }
        }
        if (d.lifetime <= 0) d.active = false;
    }
    drops.erase(std::remove_if(drops.begin(), drops.end(), [](const EquipmentDrop& d){return !d.active;}), drops.end());
}

void UpdateMaterialDrops(std::vector<MaterialDrop>& drops, Player& p, Profile& profile, float dt) {
    for (auto& d : drops) {
        if (!d.active) continue;
        d.lifetime -= dt;
        float dist = Vector2Distance(p.position, d.position);
        if (dist < 200.0f) { // Magnet
            Vector2 dir = Vector2Normalize(Vector2Subtract(p.position, d.position));
            d.position = Vector2Add(d.position, Vector2Scale(dir, 500.0f * dt));
        }
        if (dist < p.radius + 15) {
            if (d.type == 0) profile.manaCrystals += d.amount;
            else profile.monsterEssence += d.amount;
            d.active = false;
        }
        if (d.lifetime <= 0) d.active = false;
    }
    drops.erase(std::remove_if(drops.begin(), drops.end(), [](const MaterialDrop& d){return !d.active;}), drops.end());
}

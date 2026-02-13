#include "utils.h"

const char* RankName(Rank r) {
    const char* names[] = {"F", "E", "D", "C", "B", "A", "S"};
    return names[(int)r];
}

Color RankColor(Rank r) {
    Color colors[] = {GRAY, GREEN, SKYBLUE, BLUE, PURPLE, ORANGE, GOLD};
    return colors[(int)r];
}

const char* DisciplineName(Discipline d) {
    const char* names[] = {"Warrior", "Mage", "Sorcerer", "Hunter", "Summoner", "Healer"};
    int i = (int)d;
    return (i >= 0 && i < DISCIPLINE_COUNT) ? names[i] : "???";
}

Color DisciplineColor(Discipline d) {
    Color colors[] = {
        {220, 80, 60, 255},   // Warrior — red
        {100, 150, 255, 255}, // Mage — blue
        {180, 50, 200, 255},  // Sorcerer — purple
        {50, 200, 100, 255},  // Hunter — green
        {200, 200, 255, 255}, // Summoner — light blue
        {100, 255, 150, 255}, // Healer — light green
    };
    int i = (int)d;
    return (i >= 0 && i < DISCIPLINE_COUNT) ? colors[i] : WHITE;
}

const char* TierName(AbilityTier t) {
    switch (t) {
        case AbilityTier::BASIC:        return "Basic";
        case AbilityTier::INTERMEDIATE: return "Intermediate";
        case AbilityTier::ADVANCED:     return "Advanced";
        case AbilityTier::ULTIMATE:     return "Ultimate";
        default: return "???";
    }
}

const char* StatName(int index) {
    const char* names[] = {"Strength", "Agility", "Vitality", "Intelligence", "Wisdom", "Dexterity", "Spirit", "Endurance"};
    return (index >= 0 && index < STAT_COUNT) ? names[index] : "???";
}

const char* StatAbbrev(int index) {
    const char* abbrevs[] = {"STR", "AGI", "VIT", "INT", "WIS", "DEX", "SPR", "END"};
    return (index >= 0 && index < STAT_COUNT) ? abbrevs[index] : "???";
}

const char* RarityName(Rarity r) {
    const char* names[] = {"Common", "Uncommon", "Rare", "Epic", "Legendary"};
    int i = (int)r;
    return (i >= 0 && i < (int)Rarity::COUNT) ? names[i] : "???";
}

Color RarityColor(Rarity r) {
    Color colors[] = {
        {200, 200, 200, 255}, // Common — white/gray
        {80, 255, 80, 255},   // Uncommon — green
        {80, 140, 255, 255},  // Rare — blue
        {180, 80, 255, 255},  // Epic — purple
        {255, 215, 0, 255},   // Legendary — gold
    };
    int i = (int)r;
    return (i >= 0 && i < (int)Rarity::COUNT) ? colors[i] : WHITE;
}

const char* EquipSlotName(EquipSlot s) {
    const char* names[] = {"Weapon", "Helmet", "Armor", "Boots", "Accessory", "Accessory"};
    int i = (int)s;
    return (i >= 0 && i < EQUIP_SLOT_COUNT) ? names[i] : "???";
}

const char* WeaponTypeName(WeaponType w) {
    const char* names[] = {"Sword", "Staff", "Bow", "Dagger", "Mace", "Orb", "None"};
    return names[(int)w];
}

const RiftTheme RIFT_THEMES[] = {
    {"Crimson Hollow", {20, 8, 8, 255},   {200, 40, 40, 255},  {255, 60, 30, 80},  {255, 120, 120, 255}},
    {"Void Abyss",     {10, 5, 20, 255},   {120, 40, 200, 255}, {160, 50, 255, 80}, {200, 150, 255, 255}},
    {"Frozen Breach",  {5, 10, 20, 255},   {40, 150, 220, 255}, {80, 200, 255, 80}, {150, 220, 255, 255}},
    {"Ember Sanctum",  {25, 12, 5, 255},   {230, 120, 30, 255}, {255, 180, 50, 80}, {255, 200, 100, 255}},
    {"Shadow Depths",  {5, 5, 10, 255},    {60, 60, 80, 255},   {100, 80, 120, 80}, {140, 130, 160, 255}},
};
const int THEME_COUNT = 5;

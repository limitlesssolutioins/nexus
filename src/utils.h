#pragma once
#include "types.h"

const char* RankName(Rank r);
Color RankColor(Rank r);
const char* DisciplineName(Discipline d);
Color DisciplineColor(Discipline d);
const char* TierName(AbilityTier t);
const char* StatName(int index);
const char* StatAbbrev(int index);
const char* RarityName(Rarity r);
Color RarityColor(Rarity r);
const char* EquipSlotName(EquipSlot s);
const char* WeaponTypeName(WeaponType w);

extern const RiftTheme RIFT_THEMES[];
extern const int THEME_COUNT;

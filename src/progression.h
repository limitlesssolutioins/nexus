#pragma once
#include "types.h"

// Master table of all 42 abilities (6 disciplines x 7 per discipline)
extern const AbilityDef ALL_ABILITIES[TOTAL_ABILITIES];

const AbilityDef& GetAbility(int id);
void GetDisciplineAbilities(Discipline d, int outIds[ABILITIES_PER_DISCIPLINE]);

// Player creation and stat computation
Player MakePlayer(const Profile& profile);
void RecalculateStats(Player& p, const Profile& profile);

// XP and leveling (persistent)
bool AwardXP(Player& p, Profile& profile, int amount);

// Ability unlock (checks stat gates + skill points)
bool CanUnlockAbility(const Profile& profile, const Player& p, int abilityId);
bool TryUnlockAbility(Profile& profile, Player& p, int abilityId);
bool EquipAbility(Profile& profile, Player& p, int abilityId, int slot);
void UnequipSlot(Profile& profile, Player& p, int slot);

// Equipment
bool EquipItem(Profile& profile, Player& p, int inventoryIndex);
void UnequipItem(Profile& profile, Player& p, EquipSlot slot);
int FindFreeInventorySlot(const Profile& profile);

// Passive updates
void UpdatePassives(Player& p, float dt);

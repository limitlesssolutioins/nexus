#pragma once
#include "types.h"
#include <vector>

Equipment GenerateEquipmentDrop(int riftRank, int enemyLevel, EnemyType sourceType);
void TrySpawnEquipmentDrop(std::vector<EquipmentDrop>& drops, Vector2 pos, int riftRank, int enemyLevel, EnemyType sourceType);
void SpawnMaterialDrop(std::vector<MaterialDrop>& drops, Vector2 pos, int type, int amount);
void UpdateDrops(std::vector<EquipmentDrop>& drops, Player& p, Profile& profile, float dt);
void UpdateMaterialDrops(std::vector<MaterialDrop>& drops, Player& p, Profile& profile, float dt);

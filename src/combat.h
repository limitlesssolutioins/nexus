#pragma once
#include "types.h"

void ApplyDamageToPlayer(GameWorld& world, float rawDamage, bool isHazard);
void UpdateEnemies(GameWorld& world, float dt);
void UpdateEnemyDebuffs(GameWorld& world, float dt);
void UpdateProjectiles(std::vector<Projectile>& projectiles, float dt, float worldWidth, float worldHeight);
void UpdateVFX(std::vector<VFX>& vfx, float dt);
void UpdateParticles(std::vector<Particle>& particles, float dt);
void SpawnAmbientParticles(std::vector<Particle>& particles, const RiftTheme& theme, float dt);
void ProcessCollisions(GameWorld& world);
void ExecuteAbility(GameWorld& world, int slotIndex);
void PerformMeleeAttack(GameWorld& world, float range, float arcAngle, float damage, float knockback);
void PerformMining(GameWorld& world);
Color EnemyColor(EnemyType type, Color tint);

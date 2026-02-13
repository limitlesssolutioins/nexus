#include "combat.h"
#include "progression.h"
#include "persistence.h"
#include "drops.h"
#include "rift.h"

// Forward declarations
static int FindNearestEnemy(GameWorld& world, Vector2 pos, float maxDist);

Color EnemyColor(EnemyType type, Color tint) {
    switch (type) {
        case EnemyType::SWARM:   return {200, 80, 80, 255};
        case EnemyType::BRUTE:   return {160, 30, 30, 255};
        case EnemyType::SPITTER: return {tint.r, 80, tint.b, 255};
        case EnemyType::PHANTOM: return {80, tint.g, tint.b, 200};
        case EnemyType::ELITE:   return {255, 180, 30, 255};
        case EnemyType::BOSS:    return {255, 50, 50, 255};
        default: return RED;
    }
}

void ApplyDamageToPlayer(GameWorld& world, float rawDamage, bool isHazard) {
    Player& pl = world.player;
    if (!isHazard && pl.dmgCooldown > 0) return;
    if (isHazard && pl.dmgCooldown > 0.1f) return;

    // Evasion buff check (buff[6])
    if (pl.buffTimers[6] > 0) {
        float roll = (float)GetRandomValue(0, 100) / 100.0f;
        if (roll < 0.80f) return; // 80% dodge during evasion
    }

    // Shield absorb first
    if (pl.shieldHp > 0) {
        if (pl.shieldHp >= rawDamage) { pl.shieldHp -= rawDamage; return; }
        rawDamage -= pl.shieldHp;
        pl.shieldHp = 0;
    }

    // Damage reduction: passive END-based + Iron Skin buff[5]
    float reduction = pl.damageReduction;
    if (pl.buffTimers[5] > 0) reduction += 0.40f;
    if (pl.buffTimers[3] > 0) reduction += 0.20f; // Ability damage reduction buff
    if (reduction > 0.80f) reduction = 0.80f; // Hard cap

    // Flat defense mitigation
    float mitigated = rawDamage - pl.flatDefense;
    if (mitigated < 1.0f) mitigated = 1.0f; // Minimum 1 damage

    float dmg = mitigated * (1.0f - reduction);
    pl.hp -= dmg;
    pl.dmgCooldown = isHazard ? 0.15f : 0.5f;

    if (pl.hp <= 0) {
        // Check for Resurrection passive (ability 39, Healer Advanced)
        if (pl.unlockedAbilities[39] && pl.buffTimers[7] <= 0) {
            pl.hp = pl.maxHp * 0.5f;
            pl.buffTimers[7] = 99999.0f; // Mark as used (once per rift)
        } else {
            pl.hp = 0;
            world.state = GameState::GAME_OVER;
        }
    }
}

// Helper: Check if line of sight is clear
static bool CheckLineOfSight(GameWorld& world, Vector2 start, Vector2 end) {
    for (const auto& obs : world.rift.obstacles) {
        Rectangle r = obs.rect;
        Vector2 topL = { r.x, r.y };
        Vector2 topR = { r.x + r.width, r.y };
        Vector2 botL = { r.x, r.y + r.height };
        Vector2 botR = { r.x + r.width, r.y + r.height };

        // Check against the 4 segments of the rectangle
        if (CheckCollisionLines(start, end, topL, topR, nullptr)) return false;
        if (CheckCollisionLines(start, end, botL, botR, nullptr)) return false;
        if (CheckCollisionLines(start, end, topL, botL, nullptr)) return false;
        if (CheckCollisionLines(start, end, topR, botR, nullptr)) return false;
    }
    return true;
}

// Helper: Apply movement with collision (Wall Sliding)
static void ApplyMovementWithCollision(Vector2& pos, float radius, Vector2 delta, const std::vector<Obstacle>& obstacles) {
    Vector2 targetPos = Vector2Add(pos, delta);

    // Check X axis movement
    Vector2 nextPosX = { targetPos.x, pos.y };
    bool colX = false;
    for (const auto& obs : obstacles) {
        if (CheckCollisionCircleRec(nextPosX, radius, obs.rect)) { colX = true; break; }
    }
    if (!colX) pos.x = nextPosX.x;

    // Check Y axis movement
    Vector2 nextPosY = { pos.x, targetPos.y };
    bool colY = false;
    for (const auto& obs : obstacles) {
        if (CheckCollisionCircleRec(nextPosY, radius, obs.rect)) { colY = true; break; }
    }
    if (!colY) pos.y = nextPosY.y;
}

// --- Flow Field Pathfinding ---
static int flowField[DUNGEON_WIDTH][DUNGEON_HEIGHT];
static float flowFieldTimer = 0;

static void UpdateFlowField(Vector2 targetPos) {
    // Reset field
    for(int x=0; x<DUNGEON_WIDTH; x++)
        for(int y=0; y<DUNGEON_HEIGHT; y++)
            flowField[x][y] = 9999;

    int startX = (int)(targetPos.x / TILE_SIZE);
    int startY = (int)(targetPos.y / TILE_SIZE);
    
    if (IsWall(startX, startY)) return; // Player inside wall? (Shouldn't happen)

    struct Node { int x, y, dist; };
    std::vector<Node> q;
    q.reserve(DUNGEON_WIDTH * DUNGEON_HEIGHT); // Avoid reallocation
    
    q.push_back({startX, startY, 0});
    flowField[startX][startY] = 0;
    
    int head = 0;
    // BFS limit distance to avoid processing whole map every frame (e.g. 30 tiles radius)
    while(head < (int)q.size()) {
        Node curr = q[head++];
        // Limit removed to allow global pathfinding
        
        const int dx[] = {0, 0, 1, -1, 1, 1, -1, -1};
        const int dy[] = {1, -1, 0, 0, 1, -1, 1, -1};
        
        for(int i=0; i<8; i++) {
            int nx = curr.x + dx[i];
            int ny = curr.y + dy[i];
            
            if (!IsWall(nx, ny)) {
                int extraCost = 0;
                // Add "Safety Cost" to avoid hugging walls
                for(int k=0; k<8; k++) {
                    if (IsWall(nx + dx[k], ny + dy[k])) { extraCost = 20; break; }
                }

                int newDist = curr.dist + ((i < 4) ? 10 : 14) + extraCost;
                if (newDist < flowField[nx][ny]) {
                    flowField[nx][ny] = newDist;
                    q.push_back({nx, ny, newDist});
                }
            }
        }
    }
}

static Vector2 GetFlowDirection(Vector2 pos) {
    int cx = (int)(pos.x / TILE_SIZE);
    int cy = (int)(pos.y / TILE_SIZE);
    
    if (IsWall(cx, cy)) return {0,0}; // Stuck inside wall

    int bestDist = flowField[cx][cy];
    Vector2 bestDir = {0,0};
    
    const int dx[] = {0, 0, 1, -1, 1, 1, -1, -1};
    const int dy[] = {1, -1, 0, 0, 1, -1, 1, -1};
    
    bool foundBetter = false;
    for(int i=0; i<8; i++) {
        int nx = cx + dx[i];
        int ny = cy + dy[i];
        if (nx >= 0 && nx < DUNGEON_WIDTH && ny >= 0 && ny < DUNGEON_HEIGHT) {
            if (flowField[nx][ny] < bestDist) {
                bestDist = flowField[nx][ny];
                bestDir = {(float)dx[i], (float)dy[i]};
                foundBetter = true;
            }
        }
    }
    
    if (foundBetter) return Vector2Normalize(bestDir);
    return {0,0}; // Local minima or no path
}

// --- Enemy AI ---
void UpdateEnemies(GameWorld& world, float dt) {
    Player& p = world.player;
    
    // Update Flow Field periodically (every 0.2s)
    flowFieldTimer -= dt;
    if (flowFieldTimer <= 0) {
        UpdateFlowField(p.position);
        flowFieldTimer = 0.2f;
    }

    // AI Loop
    for (int i = 0; i < (int)world.enemies.size(); i++) {
        Enemy& e = world.enemies[i];
        if (!e.active) continue;

        e.alertCooldown -= dt;

        float dist = Vector2Distance(p.position, e.position);
        Vector2 toPlayer = Vector2Normalize(Vector2Subtract(p.position, e.position));
        Vector2 movement = {0, 0};
        e.alerting = false;

        // Immobilized check
        if (e.immobilized && e.immobileTimer > 0) {
            e.immobileTimer -= dt;
            if (e.immobileTimer <= 0) e.immobilized = false;
            continue;
        }

        // --- SENSORS ---
        bool canSee = false;
        bool canHear = false;
        
        // Target Selection
        Vector2 targetPos = p.position;
        bool targetIsSummon = false;
        int targetSummonIdx = -1;
        float distToTarget = dist;

        // Check summons priority
        for (int sIdx = 0; sIdx < MAX_SUMMONS; sIdx++) {
            if (!p.summons[sIdx].active) continue;
            float dSummon = Vector2Distance(e.position, p.summons[sIdx].position);
            // Taunt logic or just closer
            if (dSummon < distToTarget || p.summons[sIdx].summonType == 1) { // Type 1 is Guardian (Taunt)
                if (CheckLineOfSight(world, e.position, p.summons[sIdx].position)) {
                    distToTarget = dSummon;
                    targetPos = p.summons[sIdx].position;
                    targetIsSummon = true;
                    targetSummonIdx = sIdx;
                }
            }
        }
        
        // Hearing
        if (distToTarget < 200.0f) { // Increased hearing range
             canHear = true;
             e.alerting = true;
        }

        // Vision
        Vector2 toTarget = Vector2Normalize(Vector2Subtract(targetPos, e.position));
        if (distToTarget < e.visionRange) {
            float dot = Vector2DotProduct(e.facing, toTarget);
            if (dot > 0.4f) { // Wider vision cone (approx 130 deg)
                if (CheckLineOfSight(world, e.position, targetPos)) {
                    canSee = true;
                }
            }
        }

        float effectiveSpeed = e.speed * e.slowFactor;

        // --- STATE MACHINE ---

        // 1. Alert Propagation (The "Shout")
        if (e.state != EnemyState::CHASING && (canSee || canHear)) {
            e.state = EnemyState::CHASING;
            if (e.alertCooldown <= 0) {
                // Alert nearby friends
                for (int j = 0; j < (int)world.enemies.size(); j++) {
                    if (i == j || !world.enemies[j].active) continue;
                    if (Vector2Distance(e.position, world.enemies[j].position) < 300) {
                        world.enemies[j].state = EnemyState::CHASING;
                        world.enemies[j].alerting = true;
                    }
                }
                e.alertCooldown = 5.0f;
            }
        }

        if (e.state == EnemyState::IDLE) {
            e.patrolTimer -= dt;
            if (e.patrolTimer <= 0) {
                // Pick a random patrol point
                for(int attempt=0; attempt<5; attempt++) {
                    Vector2 rnd = { (float)GetRandomValue(-300, 300), (float)GetRandomValue(-300, 300) };
                    Vector2 dest = Vector2Add(e.position, rnd);
                    // Check bounds and walls
                    int tx = (int)(dest.x / TILE_SIZE);
                    int ty = (int)(dest.y / TILE_SIZE);
                    if (!IsWall(tx, ty)) {
                        e.targetPatrolPos = dest;
                        e.state = EnemyState::WANDERING;
                        break;
                    }
                }
                e.patrolTimer = GetRandomValue(2, 5); // Wait time before next move
            }
        }
        else if (e.state == EnemyState::WANDERING) {
            Vector2 toDest = Vector2Subtract(e.targetPatrolPos, e.position);
            float dDest = Vector2Length(toDest);
            if (dDest < 10 || canSee) {
                e.state = canSee ? EnemyState::CHASING : EnemyState::IDLE;
                e.patrolTimer = GetRandomValue(2, 4); // Rest after walking
            } else {
                movement = Vector2Scale(Vector2Normalize(toDest), effectiveSpeed * 0.3f * dt); // Walk slower
                e.facing = Vector2Normalize(toDest);
            }
        }
        else if (e.state == EnemyState::CHASING) {
            bool useFlow = !targetIsSummon; 
            if (canSee && distToTarget < 200) useFlow = false; 

            Vector2 moveDir = toTarget;
            
            // Kiting logic
            if (e.type == EnemyType::SPITTER && distToTarget < 250 && canSee) {
                moveDir = Vector2Scale(toTarget, -1.0f);
                useFlow = false;
            }
            else if (useFlow) {
                Vector2 flowDir = GetFlowDirection(e.position);
                if (Vector2Length(flowDir) > 0.1f) moveDir = flowDir;
            }

            // Flocking: Separation (Don't stack on top of each other)
            Vector2 separation = {0,0};
            int neighbors = 0;
            for (int j = 0; j < (int)world.enemies.size(); j++) {
                if (i == j || !world.enemies[j].active) continue;
                float d = Vector2Distance(e.position, world.enemies[j].position);
                if (d < e.radius * 2.5f) { // Too close!
                    Vector2 push = Vector2Normalize(Vector2Subtract(e.position, world.enemies[j].position));
                    separation = Vector2Add(separation, Vector2Scale(push, (e.radius * 3.0f - d) / e.radius));
                    neighbors++;
                    if (neighbors > 3) break; // Optimization limit
                }
            }
            
            if (neighbors > 0) {
                // Blend desire (moveDir) with separation
                moveDir = Vector2Add(moveDir, Vector2Scale(separation, 1.5f)); // Separation weight
                moveDir = Vector2Normalize(moveDir);
            }

            movement = Vector2Scale(moveDir, effectiveSpeed * dt);
            e.facing = moveDir;

            // Attack Logic
            float attackRange = (e.type == EnemyType::SPITTER) ? 250.0f : (e.radius + 50.0f);
            if (canSee && distToTarget < attackRange) { e.state = EnemyState::ATTACKING; e.stateTimer = 0.6f; }
            if (distToTarget > e.visionRange * 2.0f) e.state = EnemyState::IDLE; // Lost interest
        }
        else if (e.state == EnemyState::ATTACKING) {
            e.stateTimer -= dt;
            // Melee enemies lunge toward target during windup
            if (e.type != EnemyType::SPITTER) {
                Vector2 lunge = Vector2Scale(Vector2Normalize(Vector2Subtract(targetPos, e.position)), effectiveSpeed * 0.5f * dt);
                ApplyMovementWithCollision(e.position, e.radius, lunge, world.rift.obstacles);
            }
            if (e.stateTimer <= 0) {
                if (e.type == EnemyType::SPITTER) {
                    Vector2 dir = Vector2Normalize(Vector2Subtract(targetPos, e.position));
                    world.projectiles.push_back({e.position, Vector2Scale(dir, 400.0f), 8, e.damage * e.weaknessFactor, true, false, true, true});
                } else {
                    if (distToTarget < e.radius + 60) { // Range based on enemy size
                        if (targetIsSummon) {
                             p.summons[targetSummonIdx].hp -= e.damage * e.weaknessFactor;
                             if (p.summons[targetSummonIdx].hp <= 0) p.summons[targetSummonIdx].active = false;
                        } else {
                             ApplyDamageToPlayer(world, e.damage * e.weaknessFactor, false);
                        }
                    }
                }
                e.state = EnemyState::CHASING;
            }
        }

        ApplyMovementWithCollision(e.position, e.radius, movement, world.rift.obstacles);

        e.position.x = Clamp(e.position.x, ARENA_MARGIN, world.rift.worldWidth - ARENA_MARGIN);
        e.position.y = Clamp(e.position.y, ARENA_MARGIN, world.rift.worldHeight - ARENA_MARGIN);
    }
    
    // Update Summons
    for (int i = 0; i < MAX_SUMMONS; i++) {
        Summon& s = p.summons[i];
        if (!s.active) continue;

        s.lifetime -= dt;
        if (s.lifetime <= 0 || s.hp <= 0) { s.active = false; continue; }

        Vector2 target = p.position; 
        int enemyIdx = FindNearestEnemy(world, s.position, 600);
        
        if (enemyIdx != -1) {
            target = world.enemies[enemyIdx].position;
            if (Vector2Distance(s.position, target) < s.radius + world.enemies[enemyIdx].radius + 15) {
                 world.enemies[enemyIdx].hp -= s.damage * dt; 
                 if (world.enemies[enemyIdx].hp <= 0) {
                     world.enemies[enemyIdx].active = false;
                     world.profile.money += 20;
                     AwardXP(p, world.profile, 20);
                     TrySpawnEquipmentDrop(world.drops, world.enemies[enemyIdx].position, (int)world.rift.rank, world.enemies[enemyIdx].level, world.enemies[enemyIdx].type);
                     SpawnMaterialDrop(world.materialDrops, world.enemies[enemyIdx].position, 1, 1); // Essence
                     p.killCount++;
                 }
            }
        } else {
            if (Vector2Distance(s.position, p.position) < 100) target = s.position; 
        }

        Vector2 dir = Vector2Normalize(Vector2Subtract(target, s.position));
        if (Vector2Length(dir) > 0) {
            Vector2 move = Vector2Scale(dir, s.speed * dt);
            ApplyMovementWithCollision(s.position, s.radius, move, world.rift.obstacles);
        }
    }
}

void UpdateEnemyDebuffs(GameWorld& world, float dt) {
    for (auto& e : world.enemies) {
        if (!e.active) continue;
        // Slow
        if (e.slowTimer > 0) { e.slowTimer -= dt; if (e.slowTimer <= 0) e.slowFactor = 1.0f; }
        // DoT
        if (e.dotTimer > 0) {
            e.dotTimer -= dt;
            e.hp -= e.dotDamage * dt;
            if (e.hp <= 0) {
                e.active = false;
                world.profile.money += 20;
                AwardXP(world.player, world.profile, 20);
                TrySpawnEquipmentDrop(world.drops, e.position, (int)world.rift.rank, e.level, e.type);
                SpawnMaterialDrop(world.materialDrops, e.position, 1, 1); // Essence
            }
        }
        // Weakness
        if (e.weaknessTimer > 0) { e.weaknessTimer -= dt; if (e.weaknessTimer <= 0) e.weaknessFactor = 1.0f; }
        // Immobilize
        if (e.immobileTimer > 0) { e.immobileTimer -= dt; if (e.immobileTimer <= 0) e.immobilized = false; }
    }
}

// --- Collisions ---
void ProcessCollisions(GameWorld& world) {
    Player& pl = world.player;

    // Hazard damage
    for (const auto& obs : world.rift.obstacles) {
        if (obs.isHazard && CheckCollisionCircleRec(pl.position, pl.radius, obs.rect)) {
            ApplyDamageToPlayer(world, 1.5f, true);
        }
    }

    // Projectile collisions
    for (auto& proj : world.projectiles) {
        if (!proj.active) continue;
        // Projectile vs obstacles
        for (const auto& obs : world.rift.obstacles)
            if (CheckCollisionCircleRec(proj.position, proj.radius, obs.rect)) {
                if (!proj.isPiercing) proj.active = false;
                break;
            }
        if (!proj.active) continue;

        if (proj.isEnemy) {
            // Enemy projectile vs player
            if (Vector2Distance(proj.position, pl.position) < proj.radius + pl.radius) {
                ApplyDamageToPlayer(world, proj.damage, false);
                proj.active = false;
            }
        } else {
            // Player projectile vs enemies
            float dmgMult = proj.isMagic ? pl.magicDamageMultiplier : pl.physDamageMultiplier;
            if (pl.buffTimers[4] > 0) dmgMult *= 1.20f; // War Cry buff
            dmgMult += pl.bonusDamage;

            for (auto& e : world.enemies) {
                if (!e.active) continue;
                if (Vector2Distance(proj.position, e.position) < proj.radius + e.radius) {
                    // Crit roll
                    float critMult = 1.0f;
                    float roll = (float)GetRandomValue(0, 100) / 100.0f;
                    if (roll < pl.critChance) critMult = pl.critMultiplier;

                    float finalDmg = proj.damage * dmgMult * critMult;
                    e.hp -= finalDmg;

                    // Lifesteal
                    if (pl.lifestealRatio > 0) {
                        pl.hp = fminf(pl.hp + finalDmg * pl.lifestealRatio, pl.maxHp);
                    }

                    // VFX on hit
                    Color hitColor = (critMult > 1.0f) ? Color{255, 255, 50, 255} : Color{255, 200, 100, 255};
                    world.vfx.push_back({e.position, 5, 20, 0.3f, 0.3f, hitColor, true});

                    if (e.hp <= 0) {
                        e.active = false;
                        world.profile.money += 20;
                        AwardXP(world.player, world.profile, 20);
                        TrySpawnEquipmentDrop(world.drops, e.position, (int)world.rift.rank, e.level, e.type);
                        SpawnMaterialDrop(world.materialDrops, e.position, 1, 1);
                        pl.killCount++;
                    }

                    if (!proj.isPiercing) { proj.active = false; break; }
                }
            }
        }
    }
}

// --- Abilities ---
// Helper: spawn directional projectile
static void SpawnProjectile(GameWorld& world, Vector2 pos, Vector2 dir, float speed, float dmg, float radius, bool magic, bool piercing) {
    world.projectiles.push_back({pos, Vector2Scale(dir, speed), radius, dmg, magic, piercing, false, true});
}

// Helper: AoE damage around a point
static void DealAoEDamage(GameWorld& world, Vector2 center, float radius, float damage, bool magic) {
    Player& pl = world.player;
    float dmgMult = magic ? pl.magicDamageMultiplier : pl.physDamageMultiplier;
    for (auto& e : world.enemies) {
        if (!e.active) continue;
        if (Vector2Distance(center, e.position) < radius + e.radius) {
            float critMult = 1.0f;
            if ((float)GetRandomValue(0, 100) / 100.0f < pl.critChance) critMult = pl.critMultiplier;
            float finalDmg = damage * dmgMult * critMult;
            e.hp -= finalDmg;
            if (pl.lifestealRatio > 0) pl.hp = fminf(pl.hp + finalDmg * pl.lifestealRatio, pl.maxHp);
            world.vfx.push_back({e.position, 5, 15, 0.2f, 0.2f, {255, 150, 50, 255}, true});
            if (e.hp <= 0) {
                e.active = false;
                world.profile.money += 20;
                AwardXP(world.player, world.profile, 20);
                TrySpawnEquipmentDrop(world.drops, e.position, (int)world.rift.rank, e.level, e.type);
                SpawnMaterialDrop(world.materialDrops, e.position, 1, 1);
                pl.killCount++;
            }
        }
    }
}

// Helper: apply slow debuff to enemies in radius
static void ApplySlowInRadius(GameWorld& world, Vector2 center, float radius, float factor, float duration) {
    for (auto& e : world.enemies) {
        if (!e.active) continue;
        if (Vector2Distance(center, e.position) < radius + e.radius) {
            e.slowFactor = factor;
            e.slowTimer = duration;
        }
    }
}

// Helper: apply DoT to enemies in radius
static void ApplyDoTInRadius(GameWorld& world, Vector2 center, float radius, float dps, float duration) {
    for (auto& e : world.enemies) {
        if (!e.active) continue;
        if (Vector2Distance(center, e.position) < radius + e.radius) {
            e.dotDamage = dps;
            e.dotTimer = duration;
        }
    }
}

// Helper: immobilize enemies in radius
static void ImmobilizeInRadius(GameWorld& world, Vector2 center, float radius, float duration) {
    for (auto& e : world.enemies) {
        if (!e.active) continue;
        if (Vector2Distance(center, e.position) < radius + e.radius) {
            e.immobilized = true;
            e.immobileTimer = duration;
        }
    }
}

// Helper: spawn a summon
static void SpawnSummon(Player& pl, int type, Vector2 pos, float hp, float dmg, float spd, float life, float rad) {
    for (int i = 0; i < MAX_SUMMONS; i++) {
        if (!pl.summons[i].active) {
            pl.summons[i] = {};
            pl.summons[i].position = pos;
            pl.summons[i].hp = hp;
            pl.summons[i].maxHp = hp;
            pl.summons[i].damage = dmg;
            pl.summons[i].speed = spd;
            pl.summons[i].lifetime = life;
            pl.summons[i].radius = rad;
            pl.summons[i].summonType = type;
            pl.summons[i].active = true;
            return;
        }
    }
}

// Helper: find nearest enemy to a point
static int FindNearestEnemy(GameWorld& world, Vector2 pos, float maxDist) {
    float best = maxDist;
    int bestIdx = -1;
    for (int i = 0; i < (int)world.enemies.size(); i++) {
        if (!world.enemies[i].active) continue;
        float d = Vector2Distance(pos, world.enemies[i].position);
        if (d < best) { best = d; bestIdx = i; }
    }
    return bestIdx;
}

void PerformMining(GameWorld& world) {
    Player& pl = world.player;
    Vector2 target = GetScreenToWorld2D(GetMousePosition(), world.camera);
    Vector2 dir = Vector2Normalize(Vector2Subtract(target, pl.position));
    
    // Pickaxe swing VFX
    Vector2 swingPos = Vector2Add(pl.position, Vector2Scale(dir, 30));
    world.vfx.push_back({swingPos, 15, 40, 0.15f, 0.15f, GRAY, true}); 

    for (auto& res : world.rift.resourceNodes) {
        if (!res.active) continue;
        if (Vector2Distance(pl.position, res.position) < 70) {
             Vector2 toRes = Vector2Normalize(Vector2Subtract(res.position, pl.position));
             float dot = Vector2DotProduct(dir, toRes);
             if (dot > 0.5f) { // Cone check
                 res.hp -= 25.0f;
                 world.vfx.push_back({res.position, 5, 25, 0.2f, 0.2f, {0, 255, 255, 255}, true});
                 if (res.hp <= 0) {
                     res.active = false;
                     SpawnMaterialDrop(world.materialDrops, res.position, 0, res.value * 2); // Type 0 = Crystal
                 }
             }
        }
    }
}

// Helper: Perform a melee attack (Weapon strike)
void PerformMeleeAttack(GameWorld& world, float range, float arcAngle, float damage, float knockback) {
    Player& pl = world.player;
    Vector2 target = GetScreenToWorld2D(GetMousePosition(), world.camera);
    Vector2 dir = Vector2Normalize(Vector2Subtract(target, pl.position));
    
    // Scale damage by STR/Phys
    float finalDmg = damage * pl.physDamageMultiplier;
    // Buff checks
    if (pl.buffTimers[4] > 0) finalDmg *= 1.20f; // War Cry
    finalDmg += pl.bonusDamage;

    // Visual effect: Slash arc
    float angle = atan2f(dir.y, dir.x) * RAD2DEG;
    // Add VFX particles along the arc? For now just a hit marker if hit
    
    // Check enemies in cone
    bool hitAny = false;
    for (auto& e : world.enemies) {
        if (!e.active) continue;
        float dist = Vector2Distance(pl.position, e.position);
        if (dist < range + e.radius) {
            Vector2 toEnemy = Vector2Normalize(Vector2Subtract(e.position, pl.position));
            float dot = Vector2DotProduct(dir, toEnemy);
            // arcAngle is total degrees (e.g. 90). Half is 45. Cos(45) ~ 0.707
            float minDot = cosf((arcAngle / 2.0f) * DEG2RAD);
            
            if (dot >= minDot) {
                // HIT!
                hitAny = true;
                
                // Crit roll
                float critMult = 1.0f;
                if ((float)GetRandomValue(0, 100) / 100.0f < pl.critChance) critMult = pl.critMultiplier;
                
                e.hp -= finalDmg * critMult;
                
                // Knockback
                ApplyMovementWithCollision(e.position, e.radius, Vector2Scale(toEnemy, knockback), world.rift.obstacles);
                
                // Lifesteal
                if (pl.lifestealRatio > 0) pl.hp = fminf(pl.hp + finalDmg * critMult * pl.lifestealRatio, pl.maxHp);
                
                // VFX
                Color hitColor = (critMult > 1.0f) ? GOLD : WHITE;
                world.vfx.push_back({e.position, 10, 30, 0.2f, 0.2f, hitColor, true});
                
                if (e.hp <= 0) {
                     e.active = false;
                     world.profile.money += 20;
                     AwardXP(pl, world.profile, 20);
                     TrySpawnEquipmentDrop(world.drops, e.position, (int)world.rift.rank, e.level, e.type);
                     SpawnMaterialDrop(world.materialDrops, e.position, 1, 1);
                     pl.killCount++;
                }
            }
        }
    }

    // Visual slash indicator (using simple line/arc logic in VFX system later, for now simple particle)
    if (hitAny) {
        // Impact sound?
    }
}

void ExecuteAbility(GameWorld& world, int slotIndex) {
    Player& pl = world.player;
    AbilitySlot& slot = pl.slots[slotIndex];
    if (slot.abilityId < 0) return;

    const AbilityDef& ability = GetAbility(slot.abilityId);
    slot.cooldownTimer = slot.cooldownMax;

    Vector2 target = GetScreenToWorld2D(GetMousePosition(), world.camera);
    Vector2 dir = Vector2Normalize(Vector2Subtract(target, pl.position));
    float statValue = (ability.scalingStatIndex >= 0) ? (float)pl.stats.GetByIndex(ability.scalingStatIndex) : 0;
    float scaledValue = ability.baseValue + statValue * ability.scalingFactor;

    switch (slot.abilityId) {

        // ============================================================
        // WARRIOR (0-6) — STR / END
        // ============================================================

        case 0: // Cleave — wide melee arc
            DealAoEDamage(world, Vector2Add(pl.position, Vector2Scale(dir, 40)), 80, scaledValue, false);
            world.vfx.push_back({Vector2Add(pl.position, Vector2Scale(dir, 40)), 10, 80, 0.3f, 0.3f, {220, 80, 60, 200}, true});
            break;

        case 1: // War Cry — +20% damage buff + aggro draw
            pl.bonusDamage = 0.20f;
            pl.buffTimers[4] = 8.0f;
            world.vfx.push_back({pl.position, 10, 120, 0.5f, 0.5f, {200, 60, 40, 150}, true});
            for (auto& e : world.enemies)
                if (e.active && Vector2Distance(pl.position, e.position) < 300) e.state = EnemyState::CHASING;
            break;

        case 2: // Whirlwind — spin attack, 3 hits in expanding radius
        {
            float r = 100;
            DealAoEDamage(world, pl.position, r, scaledValue * 0.4f, false);
            DealAoEDamage(world, pl.position, r + 20, scaledValue * 0.3f, false);
            DealAoEDamage(world, pl.position, r + 40, scaledValue * 0.3f, false);
            world.vfx.push_back({pl.position, 20, 140, 0.5f, 0.5f, {230, 90, 50, 180}, true});
            break;
        }

        case 3: // Iron Skin — 40% damage reduction for 5s
            pl.buffTimers[5] = 5.0f;
            world.vfx.push_back({pl.position, 10, 35, 0.3f, 0.3f, {180, 160, 120, 200}, true});
            break;

        case 4: // Execution Strike — massive hit, +100% on low HP enemies
        {
            Vector2 hitPos = Vector2Add(pl.position, Vector2Scale(dir, 50));
            for (auto& e : world.enemies) {
                if (!e.active) continue;
                if (Vector2Distance(hitPos, e.position) < 70 + e.radius) {
                    float mult = (e.hp < e.maxHp * 0.30f) ? 2.0f : 1.0f;
                    float dmg = scaledValue * pl.physDamageMultiplier * mult;
                    float critMult = ((float)GetRandomValue(0, 100) / 100.0f < pl.critChance) ? pl.critMultiplier : 1.0f;
                    e.hp -= dmg * critMult;
                    Color c = (mult > 1.0f) ? Color{255, 50, 30, 255} : Color{255, 150, 50, 255};
                    world.vfx.push_back({e.position, 10, 40, 0.3f, 0.3f, c, true});
                    if (e.hp <= 0) { e.active = false; world.profile.money += 20; AwardXP(pl, world.profile, 20); pl.killCount++; }
                }
            }
            world.vfx.push_back({hitPos, 15, 70, 0.3f, 0.3f, {255, 50, 30, 200}, true});
            break;
        }

        case 5: // Unbreakable — immune knockback, +30% END scaling for 8s
            pl.buffTimers[5] = 8.0f; // Reuse iron skin buff slot (stronger)
            pl.buffTimers[3] = 8.0f; // Extra damage reduction
            world.vfx.push_back({pl.position, 10, 40, 0.5f, 0.5f, {160, 140, 100, 200}, true});
            break;

        case 6: // Berserker Rage — 2x STR scaling, +50% speed, -3% HP/s for 12s
            pl.bonusDamage = 1.0f; // Double damage
            pl.buffTimers[4] = 12.0f;
            pl.speed *= 1.5f;
            // Drain HP via regen being negative (handled: buff[0] with negative regen)
            pl.regenAmount = -pl.maxHp * 0.03f;
            pl.regenTimer = 1.0f;
            pl.buffTimers[0] = 12.0f;
            world.vfx.push_back({pl.position, 20, 60, 0.6f, 0.6f, {255, 30, 10, 220}, true});
            break;

        // ============================================================
        // MAGE (7-13) — INT / WIS
        // ============================================================

        case 7: // Arcane Bolt — fast magic projectile
            SpawnProjectile(world, pl.position, dir, 600, scaledValue, 10, true, false);
            break;

        case 8: // Frost Nova — AoE slow 50% for 4s + damage
            ApplySlowInRadius(world, pl.position, 150, 0.50f, 4.0f);
            DealAoEDamage(world, pl.position, 150, scaledValue, true);
            world.vfx.push_back({pl.position, 10, 150, 0.4f, 0.4f, {150, 220, 255, 180}, true});
            break;

        case 9: // Fireball — explosive projectile + burn DoT
        {
            SpawnProjectile(world, pl.position, dir, 500, scaledValue, 15, true, false);
            // Apply burn DoT to enemies near impact direction
            Vector2 impactPos = Vector2Add(pl.position, Vector2Scale(dir, 200));
            ApplyDoTInRadius(world, impactPos, 80, scaledValue * 0.15f, 4.0f);
            world.vfx.push_back({impactPos, 10, 80, 0.5f, 0.5f, {255, 120, 30, 200}, true});
            break;
        }

        case 10: // Lightning Chain — hit target, chain to 3 nearby
        {
            int first = FindNearestEnemy(world, Vector2Add(pl.position, Vector2Scale(dir, 100)), 300);
            if (first >= 0) {
                float dmg = scaledValue * pl.magicDamageMultiplier;
                world.enemies[first].hp -= dmg;
                world.vfx.push_back({world.enemies[first].position, 5, 25, 0.2f, 0.2f, {255, 255, 100, 255}, true});
                if (world.enemies[first].hp <= 0) { world.enemies[first].active = false; world.profile.money += 20; AwardXP(pl, world.profile, 20); pl.killCount++; }
                // Chain to 3 more
                Vector2 chainPos = world.enemies[first].position;
                for (int c = 0; c < 3; c++) {
                    float bestDist = 200;
                    int bestIdx = -1;
                    for (int j = 0; j < (int)world.enemies.size(); j++) {
                        if (!world.enemies[j].active || j == first) continue;
                        float d = Vector2Distance(chainPos, world.enemies[j].position);
                        if (d < bestDist) { bestDist = d; bestIdx = j; }
                    }
                    if (bestIdx >= 0) {
                        world.enemies[bestIdx].hp -= dmg * 0.6f;
                        world.vfx.push_back({world.enemies[bestIdx].position, 5, 20, 0.15f, 0.15f, {255, 255, 100, 200}, true});
                        if (world.enemies[bestIdx].hp <= 0) { world.enemies[bestIdx].active = false; world.profile.money += 20; AwardXP(pl, world.profile, 20); pl.killCount++; }
                        chainPos = world.enemies[bestIdx].position;
                    } else break;
                }
            }
            break;
        }

        case 11: // Meteor — delayed massive AoE (instant for now, TODO: delay)
        {
            Vector2 impactPos = Vector2Add(pl.position, Vector2Scale(dir, 250));
            DealAoEDamage(world, impactPos, 120, scaledValue, true);
            world.vfx.push_back({impactPos, 20, 120, 0.6f, 0.6f, {255, 80, 20, 230}, true});
            world.vfx.push_back({impactPos, 5, 60, 0.4f, 0.4f, {255, 200, 50, 180}, true});
            break;
        }

        case 12: // Mana Shield — convert damage to cooldown increases for 6s
            pl.shieldHp = 50.0f + pl.stats.WIS * 8.0f;
            pl.buffTimers[1] = 6.0f;
            world.vfx.push_back({pl.position, 10, 35, 0.4f, 0.4f, {120, 180, 255, 200}, true});
            break;

        case 13: // Arcane Singularity — black hole pulls+damages 6s (instant burst for now)
        {
            Vector2 center = Vector2Add(pl.position, Vector2Scale(dir, 200));
            // Pull enemies toward center
            for (auto& e : world.enemies) {
                if (!e.active) continue;
                float d = Vector2Distance(center, e.position);
                if (d < 200) {
                    Vector2 pull = Vector2Normalize(Vector2Subtract(center, e.position));
                    ApplyMovementWithCollision(e.position, e.radius, Vector2Scale(pull, 80), world.rift.obstacles);
                }
            }
            DealAoEDamage(world, center, 150, scaledValue, true);
            world.vfx.push_back({center, 30, 150, 0.8f, 0.8f, {80, 50, 220, 200}, true});
            world.vfx.push_back({center, 10, 80, 0.6f, 0.6f, {120, 80, 255, 180}, true});
            break;
        }

        // ============================================================
        // SORCERER (14-20) — INT / SPR
        // ============================================================

        case 14: // Hex Bolt — projectile + -15% enemy damage debuff
        {
            SpawnProjectile(world, pl.position, dir, 500, scaledValue, 10, true, false);
            for (auto& e : world.enemies) {
                if (!e.active) continue;
                if (Vector2Distance(pl.position, e.position) < 300) {
                    Vector2 toE = Vector2Normalize(Vector2Subtract(e.position, pl.position));
                    if (Vector2DotProduct(dir, toE) > 0.7f) {
                        e.weaknessFactor = 0.85f;
                        e.weaknessTimer = 5.0f;
                        break;
                    }
                }
            }
            break;
        }

        case 15: // Drain Touch — short range damage + heal 30%
        {
            float totalDrained = 0;
            for (auto& e : world.enemies) {
                if (!e.active) continue;
                if (Vector2Distance(pl.position, e.position) < 60 + e.radius) {
                    float dmg = scaledValue * pl.magicDamageMultiplier;
                    e.hp -= dmg;
                    totalDrained += dmg;
                    world.vfx.push_back({e.position, 5, 20, 0.3f, 0.3f, {160, 30, 180, 200}, true});
                    if (e.hp <= 0) { e.active = false; world.profile.money += 20; AwardXP(pl, world.profile, 20); pl.killCount++; }
                }
            }
            pl.hp = fminf(pl.hp + totalDrained * 0.30f * pl.healPower, pl.maxHp);
            break;
        }

        case 16: // Curse of Weakness — AoE -25% enemy speed and damage for 8s
            ApplySlowInRadius(world, pl.position, 200, 0.75f, 8.0f);
            for (auto& e : world.enemies) {
                if (!e.active) continue;
                if (Vector2Distance(pl.position, e.position) < 200 + e.radius) {
                    e.weaknessFactor = 0.75f;
                    e.weaknessTimer = 8.0f;
                }
            }
            world.vfx.push_back({pl.position, 15, 200, 0.5f, 0.5f, {140, 20, 160, 150}, true});
            break;

        case 17: // Soul Burn — DoT 5% maxHP/s for 6s
        {
            int idx = FindNearestEnemy(world, Vector2Add(pl.position, Vector2Scale(dir, 100)), 400);
            if (idx >= 0) {
                Enemy& e = world.enemies[idx];
                e.dotDamage = e.maxHp * 0.05f + scaledValue * 0.1f;
                e.dotTimer = 6.0f;
                world.vfx.push_back({e.position, 5, 25, 0.4f, 0.4f, {200, 60, 220, 200}, true});
            }
            break;
        }

        case 18: // Void Prison — immobilize 3s, damage on release
        {
            int idx = FindNearestEnemy(world, Vector2Add(pl.position, Vector2Scale(dir, 150)), 300);
            if (idx >= 0) {
                Enemy& e = world.enemies[idx];
                e.immobilized = true;
                e.immobileTimer = 3.0f;
                e.hp -= scaledValue * pl.magicDamageMultiplier * 0.5f; // Initial damage
                world.vfx.push_back({e.position, 10, 40, 3.0f, 3.0f, {120, 10, 180, 180}, true});
                // Delayed damage will happen via DoT
                e.dotDamage = scaledValue * pl.magicDamageMultiplier * 0.2f;
                e.dotTimer = 3.0f;
                if (e.hp <= 0) { e.active = false; world.profile.money += 20; AwardXP(pl, world.profile, 20); pl.killCount++; }
            }
            break;
        }

        case 19: // Life Siphon — continuous drain+heal (instant burst version)
        {
            float totalDrained = 0;
            for (auto& e : world.enemies) {
                if (!e.active) continue;
                if (Vector2Distance(pl.position, e.position) < 150 + e.radius) {
                    float dmg = scaledValue * pl.magicDamageMultiplier * 0.5f;
                    e.hp -= dmg;
                    totalDrained += dmg;
                    // Apply ongoing drain
                    e.dotDamage = scaledValue * 0.15f;
                    e.dotTimer = 4.0f;
                    world.vfx.push_back({e.position, 5, 20, 0.3f, 0.3f, {200, 80, 255, 180}, true});
                    if (e.hp <= 0) { e.active = false; world.profile.money += 20; AwardXP(pl, world.profile, 20); pl.killCount++; }
                }
            }
            pl.hp = fminf(pl.hp + totalDrained * 0.5f * pl.healPower, pl.maxHp);
            pl.lifestealRatio = 0.25f;
            pl.buffTimers[2] = 4.0f;
            break;
        }

        case 20: // Domain Expansion — all enemies 2x damage taken, no heal for 10s
            for (auto& e : world.enemies) {
                if (!e.active) continue;
                if (Vector2Distance(pl.position, e.position) < 300 + e.radius) {
                    e.weaknessFactor = 2.0f;
                    e.weaknessTimer = 10.0f;
                    world.vfx.push_back({e.position, 5, 20, 0.3f, 0.3f, {100, 0, 150, 200}, true});
                }
            }
            world.vfx.push_back({pl.position, 30, 300, 1.0f, 1.0f, {100, 0, 150, 120}, true});
            break;

        // ============================================================
        // HUNTER (21-27) — AGI / DEX
        // ============================================================

        case 21: // Quick Shot — fast ranged attack
            SpawnProjectile(world, pl.position, dir, 700, scaledValue, 7, false, false);
            break;

        case 22: // Dash — quick movement + brief i-frames
            pl.position = Vector2Add(pl.position, Vector2Scale(dir, 200.0f));
            pl.position.x = Clamp(pl.position.x, 20.0f, world.rift.worldWidth - 20.0f);
            pl.position.y = Clamp(pl.position.y, 80.0f, SCREEN_H - 80.0f);
            pl.dmgCooldown = 0.3f;
            world.vfx.push_back({pl.position, 5, 30, 0.2f, 0.2f, {80, 255, 130, 150}, true});
            break;

        case 23: // Fan of Knives — 8 projectiles in spread
        {
            float baseAngle = atan2f(dir.y, dir.x);
            for (int i = 0; i < 8; i++) {
                float angle = baseAngle + (i - 3.5f) * 0.2f;
                Vector2 d = {cosf(angle), sinf(angle)};
                SpawnProjectile(world, pl.position, d, 550, scaledValue * 0.5f, 5, false, false);
            }
            break;
        }

        case 24: // Trap — place trap at cursor, roots enemy 3s + damage
        {
            Vector2 trapPos = Vector2Add(pl.position, Vector2Scale(dir, 150));
            ImmobilizeInRadius(world, trapPos, 50, 3.0f);
            DealAoEDamage(world, trapPos, 50, scaledValue, false);
            world.vfx.push_back({trapPos, 10, 50, 3.0f, 3.0f, {90, 180, 70, 180}, true});
            break;
        }

        case 25: // Assassinate — teleport behind target, 3x damage
        {
            int idx = FindNearestEnemy(world, Vector2Add(pl.position, Vector2Scale(dir, 200)), 400);
            if (idx >= 0) {
                Enemy& e = world.enemies[idx];
                Vector2 behind = Vector2Add(e.position, Vector2Scale(e.facing, -40));
                pl.position = behind;
                pl.position.x = Clamp(pl.position.x, 20.0f, world.rift.worldWidth - 20.0f);
                pl.position.y = Clamp(pl.position.y, 80.0f, SCREEN_H - 80.0f);
                float dmg = scaledValue * pl.physDamageMultiplier * 3.0f;
                float critMult = ((float)GetRandomValue(0, 100) / 100.0f < pl.critChance) ? pl.critMultiplier : 1.0f;
                e.hp -= dmg * critMult;
                world.vfx.push_back({e.position, 10, 30, 0.2f, 0.2f, {30, 160, 60, 230}, true});
                if (e.hp <= 0) { e.active = false; world.profile.money += 20; AwardXP(pl, world.profile, 20); pl.killCount++; }
                pl.dmgCooldown = 0.3f; // Brief invuln after teleport
            }
            break;
        }

        case 26: // Evasion Mastery — 80% dodge for 4s, counter on dodge
            pl.buffTimers[6] = 4.0f;
            world.vfx.push_back({pl.position, 10, 35, 0.3f, 0.3f, {70, 255, 160, 200}, true});
            break;

        case 27: // Phantom Rush — 10 rapid guaranteed crits, invulnerable during
        {
            pl.dmgCooldown = 2.0f; // Invulnerable for 2s
            int idx = FindNearestEnemy(world, pl.position, 300);
            if (idx >= 0) {
                Enemy& e = world.enemies[idx];
                for (int hit = 0; hit < 10; hit++) {
                    float dmg = scaledValue * 0.3f * pl.physDamageMultiplier * pl.critMultiplier;
                    e.hp -= dmg;
                    if (e.hp <= 0) { e.active = false; world.profile.money += 20; AwardXP(pl, world.profile, 20); pl.killCount++; break; }
                }
                world.vfx.push_back({e.position, 15, 50, 0.5f, 0.5f, {20, 255, 80, 230}, true});
                pl.position = Vector2Add(e.position, {30, 0});
            }
            break;
        }

        // ============================================================
        // SUMMONER (28-34) — SPR / INT
        // ============================================================

        case 28: // Summon Wisp — auto-attack companion
        {
            Vector2 pos = Vector2Add(pl.position, {(float)GetRandomValue(-50, 50), (float)GetRandomValue(-50, 50)});
            SpawnSummon(pl, 0, pos, scaledValue, 10 + pl.stats.SPR * 1.5f, 250, 15, 12);
            world.vfx.push_back({pos, 5, 20, 0.3f, 0.3f, {200, 200, 255, 200}, true});
            break;
        }

        case 29: // Spirit Link — mark enemies to take 30% more damage
            for (auto& e : world.enemies) {
                if (!e.active) continue;
                if (Vector2Distance(pl.position, e.position) < 200) {
                    e.weaknessFactor = 1.30f;
                    e.weaknessTimer = 8.0f;
                    world.vfx.push_back({e.position, 5, 15, 0.3f, 0.3f, {180, 180, 240, 180}, true});
                }
            }
            break;

        case 30: // Summon Guardian — tank minion with taunt
        {
            Vector2 pos = Vector2Add(pl.position, Vector2Scale(dir, 60));
            float hp = scaledValue * 2.0f;
            float dmg = 8 + pl.stats.SPR * 1.0f;
            SpawnSummon(pl, 1, pos, hp, dmg, 150, 20, 22);
            // Taunt nearby enemies
            for (auto& e : world.enemies) {
                if (e.active && Vector2Distance(pos, e.position) < 250)
                    e.state = EnemyState::CHASING;
            }
            world.vfx.push_back({pos, 10, 30, 0.4f, 0.4f, {150, 150, 220, 200}, true});
            break;
        }

        case 31: // Empower Summons — all summons +50% damage for 10s
            for (int i = 0; i < MAX_SUMMONS; i++) {
                if (pl.summons[i].active) {
                    pl.summons[i].damage *= 1.5f;
                    pl.summons[i].lifetime += 5.0f; // Extend lifetime too
                    world.vfx.push_back({pl.summons[i].position, 5, 20, 0.3f, 0.3f, {220, 220, 255, 200}, true});
                }
            }
            break;

        case 32: // Summon Shadow — clone that mirrors attacks at 60%
        {
            Vector2 pos = Vector2Add(pl.position, {(float)GetRandomValue(-40, 40), (float)GetRandomValue(-40, 40)});
            float hp = scaledValue * 1.5f;
            float dmg = (15 + pl.stats.SPR * 2.0f) * 0.6f;
            SpawnSummon(pl, 2, pos, hp, dmg, 280, 20, 18);
            world.vfx.push_back({pos, 10, 25, 0.3f, 0.3f, {100, 100, 200, 200}, true});
            break;
        }

        case 33: // Soul Storm — sacrifice all summons, each explodes AoE
        {
            for (int i = 0; i < MAX_SUMMONS; i++) {
                if (pl.summons[i].active) {
                    float explosionDmg = pl.summons[i].hp * 0.5f + scaledValue;
                    DealAoEDamage(world, pl.summons[i].position, 100, explosionDmg, true);
                    world.vfx.push_back({pl.summons[i].position, 20, 100, 0.5f, 0.5f, {180, 80, 255, 220}, true});
                    pl.summons[i].active = false;
                }
            }
            break;
        }

        case 34: // Arise — resurrect dead enemies as temporary allies (Solo Leveling)
        {
            int arisen = 0;
            for (auto& e : world.enemies) {
                if (e.active || arisen >= MAX_SUMMONS) continue;
                // "Resurrect" as summon
                SpawnSummon(pl, 2, e.position, e.maxHp * 0.5f, e.damage * 0.6f, e.speed, 15, e.radius);
                world.vfx.push_back({e.position, 10, 30, 0.5f, 0.5f, {80, 60, 180, 200}, true});
                arisen++;
            }
            world.vfx.push_back({pl.position, 30, 200, 0.8f, 0.8f, {80, 60, 180, 120}, true});
            break;
        }

        // ============================================================
        // HEALER (35-41) — WIS / SPR
        // ============================================================

        case 35: // Mend — instant WIS-scaled heal
            pl.hp = fminf(pl.hp + scaledValue * pl.healPower, pl.maxHp);
            world.vfx.push_back({pl.position, 10, 40, 0.4f, 0.4f, {100, 255, 150, 200}, true});
            break;

        case 36: // Purify — cleanse + small heal
            pl.hp = fminf(pl.hp + scaledValue * pl.healPower, pl.maxHp);
            // Clear negative buffs
            for (int i = 0; i < MAX_BUFFS; i++)
                if (pl.buffTimers[i] > 0 && pl.regenAmount < 0) { pl.regenAmount = 0; }
            world.vfx.push_back({pl.position, 10, 50, 0.4f, 0.4f, {200, 255, 200, 200}, true});
            break;

        case 37: // Regen Aura — HoT in radius for 10s
            pl.regenAmount = scaledValue * 0.3f;
            pl.regenTimer = 1.0f;
            pl.buffTimers[0] = 10.0f;
            world.vfx.push_back({pl.position, 15, 80, 0.6f, 0.6f, {120, 255, 180, 150}, true});
            // Also heal summons
            for (int i = 0; i < MAX_SUMMONS; i++) {
                if (pl.summons[i].active)
                    pl.summons[i].hp = fminf(pl.summons[i].hp + scaledValue * 0.5f, pl.summons[i].maxHp);
            }
            break;

        case 38: // Holy Shield — absorb shield WIS-scaled for 8s
            pl.shieldHp = scaledValue * pl.healPower;
            pl.buffTimers[1] = 8.0f;
            world.vfx.push_back({pl.position, 10, 35, 0.4f, 0.4f, {255, 255, 100, 200}, true});
            break;

        case 39: // Resurrection — passive, auto-revive handled in ApplyDamageToPlayer
            // This is a passive — just having it unlocked triggers the effect
            // Show feedback that it's ready
            if (pl.buffTimers[7] <= 0) {
                world.vfx.push_back({pl.position, 10, 50, 0.5f, 0.5f, {255, 220, 60, 200}, true});
            }
            slot.cooldownTimer = 0; // No cooldown, it's passive
            break;

        case 40: // Sanctuary — zone where enemies deal 50% less damage for 8s
        {
            Vector2 center = pl.position;
            for (auto& e : world.enemies) {
                if (!e.active) continue;
                if (Vector2Distance(center, e.position) < 200 + e.radius) {
                    e.weaknessFactor = 0.50f; // Enemies do 50% less
                    e.weaknessTimer = 8.0f;
                }
            }
            pl.buffTimers[3] = 8.0f; // Player takes less damage too
            world.vfx.push_back({center, 20, 200, 1.0f, 1.0f, {180, 255, 220, 120}, true});
            break;
        }

        case 41: // Divine Judgment — massive heal + damage burst, scales with missing HP
        {
            float missingHpRatio = 1.0f - (pl.hp / pl.maxHp);
            float healAmount = scaledValue * pl.healPower * (1.0f + missingHpRatio);
            pl.hp = fminf(pl.hp + healAmount, pl.maxHp);
            // Damage burst to all enemies, stronger the more HP was missing
            float burstDmg = scaledValue * (1.0f + missingHpRatio * 2.0f);
            DealAoEDamage(world, pl.position, 200, burstDmg, true);
            world.vfx.push_back({pl.position, 30, 200, 0.8f, 0.8f, {255, 240, 100, 220}, true});
            world.vfx.push_back({pl.position, 10, 100, 0.5f, 0.5f, {255, 255, 200, 180}, true});
            break;
        }

        default:
            break;
    }
}

// --- Projectile & VFX Updates ---
void UpdateProjectiles(std::vector<Projectile>& projectiles, float dt, float worldWidth, float worldHeight) {
    for (auto& p : projectiles) {
        p.position = Vector2Add(p.position, Vector2Scale(p.velocity, dt));
        if (p.position.x < -100 || p.position.x > worldWidth + 100 ||
            p.position.y < -100 || p.position.y > worldHeight + 100)
            p.active = false;
    }
    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
        [](const Projectile& p) { return !p.active; }), projectiles.end());
}

void UpdateVFX(std::vector<VFX>& vfx, float dt) {
    for (auto& v : vfx) {
        v.lifetime -= dt;
        if (v.lifetime <= 0) v.active = false;
        else v.radius = Lerp(v.radius, v.maxRadius, 1.0f - (v.lifetime / v.maxLifetime));
    }
    vfx.erase(std::remove_if(vfx.begin(), vfx.end(),
        [](const VFX& v) { return !v.active; }), vfx.end());
}

void UpdateParticles(std::vector<Particle>& particles, float dt) {
    for (auto& p : particles) {
        p.position = Vector2Add(p.position, Vector2Scale(p.velocity, dt));
        p.lifetime -= dt;
        if (p.lifetime <= 0) p.active = false;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& p) { return !p.active; }), particles.end());
}

void SpawnAmbientParticles(std::vector<Particle>& particles, const RiftTheme& theme, float dt) {
    if (GetRandomValue(0, 100) < 3) {
        Particle p{};
        p.position = {(float)GetRandomValue(0, SCREEN_W * 4), (float)GetRandomValue(0, 2000)};
        p.velocity = {(float)GetRandomValue(-20, 20), (float)GetRandomValue(-40, -10)};
        p.lifetime = p.maxLifetime = 3.0f;
        p.size = (float)GetRandomValue(2, 5);
        p.color = theme.particleColor;
        p.color.a = 120;
        p.active = true;
        particles.push_back(p);
    }
}

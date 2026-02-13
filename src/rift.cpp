#include "rift.h"
#include "drops.h"
#include "progression.h"
#include "persistence.h"
#include "utils.h"
#include <vector>
#include <algorithm>

// --- Dungeon Generation Logic ---
static DungeonGen dungeonData; // Internal static data for current level

// Initialize map with noise
static void InitMapNoise(int fillPercent) {
    for (int x = 0; x < DUNGEON_WIDTH; x++) {
        for (int y = 0; y < DUNGEON_HEIGHT; y++) {
            // Edges are always walls
            if (x == 0 || x == DUNGEON_WIDTH - 1 || y == 0 || y == DUNGEON_HEIGHT - 1) {
                dungeonData.map[x][y] = 1;
            } else {
                dungeonData.map[x][y] = (GetRandomValue(0, 100) < fillPercent) ? 1 : 0;
            }
        }
    }
}

// Cellular Automata smooth step
static void SmoothMap() {
    int newMap[DUNGEON_WIDTH][DUNGEON_HEIGHT];
    for (int x = 0; x < DUNGEON_WIDTH; x++) {
        for (int y = 0; y < DUNGEON_HEIGHT; y++) {
            int wallCount = 0;
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 && nx < DUNGEON_WIDTH && ny >= 0 && ny < DUNGEON_HEIGHT) {
                        if (dungeonData.map[nx][ny] == 1) wallCount++;
                    } else {
                        wallCount++; // Outside is wall
                    }
                }
            }
            if (wallCount > 4) newMap[x][y] = 1;
            else if (wallCount < 4) newMap[x][y] = 0;
            else newMap[x][y] = dungeonData.map[x][y];
        }
    }
    // Copy back
    for (int x = 0; x < DUNGEON_WIDTH; x++)
        for (int y = 0; y < DUNGEON_HEIGHT; y++)
            dungeonData.map[x][y] = newMap[x][y];
}

bool IsWall(int x, int y) {
    if (x < 0 || x >= DUNGEON_WIDTH || y < 0 || y >= DUNGEON_HEIGHT) return true;
    return dungeonData.map[x][y] == 1;
}

// Convert grid to optimized collision rectangles (The Mesher)
static void GenerateCollisionRects(Rift& r) {
    r.obstacles.clear();
    dungeonData.collisionRects.clear();

    // Simple horizontal run-length encoding for walls
    for (int y = 0; y < DUNGEON_HEIGHT; y++) {
        int startX = -1;
        for (int x = 0; x < DUNGEON_WIDTH; x++) {
            if (dungeonData.map[x][y] == 1) {
                if (startX == -1) startX = x;
            } else {
                if (startX != -1) {
                    // Wall ended, push rect
                    Rectangle rect = {
                        (float)startX * TILE_SIZE,
                        (float)y * TILE_SIZE,
                        (float)(x - startX) * TILE_SIZE,
                        (float)TILE_SIZE
                    };
                    Obstacle obs = {rect, DARKGRAY, false};
                    r.obstacles.push_back(obs);
                    dungeonData.collisionRects.push_back(rect);
                    startX = -1;
                }
            }
        }
        // End of row check
        if (startX != -1) {
            Rectangle rect = {
                (float)startX * TILE_SIZE,
                (float)y * TILE_SIZE,
                (float)(DUNGEON_WIDTH - startX) * TILE_SIZE,
                (float)TILE_SIZE
            };
            Obstacle obs = {rect, DARKGRAY, false};
            r.obstacles.push_back(obs);
            dungeonData.collisionRects.push_back(rect);
        }
    }
}

// Generate structured rooms (BSP-like)
static void BuildStructuredDungeon() {
    // Fill with walls
    for(int x=0; x<DUNGEON_WIDTH; x++)
        for(int y=0; y<DUNGEON_HEIGHT; y++)
            dungeonData.map[x][y] = 1;

    // Create rooms
    struct Room { int x, y, w, h; };
    std::vector<Room> rooms;
    
    for (int i = 0; i < 20; i++) {
        int w = GetRandomValue(4, 10);
        int h = GetRandomValue(4, 10);
        int x = GetRandomValue(2, DUNGEON_WIDTH - w - 2);
        int y = GetRandomValue(2, DUNGEON_HEIGHT - h - 2);
        
        // Check overlap
        bool overlap = false;
        for (const auto& other : rooms) {
            if (x < other.x + other.w + 1 && x + w + 1 > other.x &&
                y < other.y + other.h + 1 && y + h + 1 > other.y) {
                overlap = true; break;
            }
        }
        if (!overlap) {
            rooms.push_back({x, y, w, h});
            // Carve room
            for (int rx = x; rx < x + w; rx++)
                for (int ry = y; ry < y + h; ry++)
                    dungeonData.map[rx][ry] = 0;
            
            // Connect to previous room (simple corridor)
            if (rooms.size() > 1) {
                Room& prev = rooms[rooms.size() - 2];
                Vector2 c1 = { (float)prev.x + prev.w/2, (float)prev.y + prev.h/2 };
                Vector2 c2 = { (float)x + w/2, (float)y + h/2 };
                
                // Horizontal corridor
                int x1 = (int)fminf(c1.x, c2.x);
                int x2 = (int)fmaxf(c1.x, c2.x);
                int yMiddle = (int)c1.y;
                for (int cx = x1; cx <= x2; cx++) dungeonData.map[cx][yMiddle] = 0;
                
                // Vertical corridor
                int y1 = (int)fminf(c1.y, c2.y);
                int y2 = (int)fmaxf(c1.y, c2.y);
                int xMiddle = (int)c2.x;
                for (int cy = y1; cy <= y2; cy++) dungeonData.map[xMiddle][cy] = 0;
            }
        }
    }
}

// Helper: Carve a guaranteed path (Drunken Walk from Start to End)
static void EnsureConnectivity() {
    int x = 5;
    int y = 5;
    int targetX = DUNGEON_WIDTH - 8;
    int targetY = DUNGEON_HEIGHT - 8;
    
    // Clear start area (Safety Zone)
    for(int i=2; i<10; i++) 
        for(int j=2; j<10; j++) 
            dungeonData.map[i][j] = 0;

    // Drunken Walk towards Target
    while (x < targetX || y < targetY) {
        // Carve current pos (3x3 brush)
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                int nx = x + dx;
                int ny = y + dy;
                if (nx > 1 && nx < DUNGEON_WIDTH - 2 && ny > 1 && ny < DUNGEON_HEIGHT - 2) {
                    dungeonData.map[nx][ny] = 0;
                }
            }
        }
        
        // Move towards target with randomness
        int r = GetRandomValue(0, 100);
        if (r < 45) { // 45% move generally towards X
             if (x < targetX) x++;
             else if (x > targetX) x--;
        }
        else if (r < 90) { // 45% move generally towards Y
             if (y < targetY) y++;
             else if (y > targetY) y--;
        } 
        else { // 10% random jitter (sidestep)
             if (GetRandomValue(0,1) == 0) x += GetRandomValue(-1, 1);
             else y += GetRandomValue(-1, 1);
        }
        
        // Clamp
        x = Clamp(x, 2, DUNGEON_WIDTH - 3);
        y = Clamp(y, 2, DUNGEON_HEIGHT - 3);
    }
    
    // Clear Boss Room
    for(int i=targetX-5; i<=targetX+5; i++) 
        for(int j=targetY-5; j<=targetY+5; j++) 
             if(i>0 && i<DUNGEON_WIDTH && j>0 && j<DUNGEON_HEIGHT) dungeonData.map[i][j] = 0;
}

// Helper: Remove disconnected pockets using Flood Fill
static void RemoveDisconnectedAreas() {
    bool visited[DUNGEON_WIDTH][DUNGEON_HEIGHT] = {false};
    std::vector<Vector2> q;
    
    // Start at safe zone center
    q.push_back({5, 5});
    visited[5][5] = true;
    
    int head = 0;
    while(head < (int)q.size()) {
        Vector2 curr = q[head++];
        int cx = (int)curr.x;
        int cy = (int)curr.y;
        
        // Check 4 neighbors
        const int dx[] = {0, 0, 1, -1};
        const int dy[] = {1, -1, 0, 0};
        
        for(int i=0; i<4; i++) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];
            
            if (nx >= 0 && nx < DUNGEON_WIDTH && ny >= 0 && ny < DUNGEON_HEIGHT) {
                if (dungeonData.map[nx][ny] == 0 && !visited[nx][ny]) {
                    visited[nx][ny] = true;
                    q.push_back({(float)nx, (float)ny});
                }
            }
        }
    }
    
    // Fill unvisited areas
    for(int x=0; x<DUNGEON_WIDTH; x++) {
        for(int y=0; y<DUNGEON_HEIGHT; y++) {
            if (dungeonData.map[x][y] == 0 && !visited[x][y]) {
                dungeonData.map[x][y] = 1; // Fill with wall
            }
        }
    }
}

static void SpawnResources(Rift& r) {
    r.resourceNodes.clear();
    int count = 0;
    int attempts = 0;
    while(count < 40 && attempts < 1000) {
        attempts++;
        int x = GetRandomValue(5, DUNGEON_WIDTH - 5);
        int y = GetRandomValue(5, DUNGEON_HEIGHT - 5);
        if (dungeonData.map[x][y] == 0) {
            ResourceNode node{};
            node.position = { (float)x * TILE_SIZE + TILE_SIZE/2, (float)y * TILE_SIZE + TILE_SIZE/2 };
            node.active = true;
            node.hp = 50.0f;
            node.value = (GetRandomValue(0, 100) < 20) ? 2 : 1; // 20% High value
            r.resourceNodes.push_back(node);
            count++;
        }
    }
}

static void SpawnDecorations(Rift& r) {
    r.decorations.clear();
    // Aim for ~300 props for a 100x100 map
    for(int i=0; i<300; i++) {
        int x = GetRandomValue(2, DUNGEON_WIDTH - 2);
        int y = GetRandomValue(2, DUNGEON_HEIGHT - 2);
        if (dungeonData.map[x][y] == 0) {
            Decoration d{};
            d.position = { (float)x * TILE_SIZE + GetRandomValue(10, 54), (float)y * TILE_SIZE + GetRandomValue(10, 54) };
            d.scale = (float)GetRandomValue(80, 120) / 100.0f;
            
            int roll = GetRandomValue(0, 100);
            if (roll < 40) d.type = 0; // Rock
            else if (roll < 70) d.type = 1; // Bones
            else if (roll < 95) d.type = 2; // Grass
            else d.type = 3; // Torch
            
            r.decorations.push_back(d);
        }
    }
}

// Generate the dungeon based on theme
static void BuildDungeon(Rift& r) {
    // 1. Clear previous
    for(int x=0; x<DUNGEON_WIDTH; x++)
        for(int y=0; y<DUNGEON_HEIGHT; y++)
            dungeonData.map[x][y] = 1; // Start filled

    // 2. Select algorithm based on Theme
    // Themes: Crimson Hollow (0), Void Abyss (1), Frozen Breach (2), Ember Sanctum (3), Shadow Depths (4)
    bool organic = true;
    // Void Abyss & Ember Sanctum -> Structured Rooms
    if (r.theme.name[0] == 'V' || r.theme.name[0] == 'E') organic = false; 

    if (organic) {
        // Init noise (48% walls)
        InitMapNoise(48);
        // Smooth 5 times for organic caves
        for (int i = 0; i < 5; i++) SmoothMap();
    } else {
        BuildStructuredDungeon();
    }

    // FORCE PATH: Ensure connectivity for ALL map types
    EnsureConnectivity();
    
    // CLEANUP: Remove isolated pockets
    RemoveDisconnectedAreas();

    // 4. Generate obstacles
    GenerateCollisionRects(r);
    
    // 5. Spawn Resources
    SpawnResources(r);
    SpawnDecorations(r);
    
    // Set world bounds based on grid
    r.worldWidth = DUNGEON_WIDTH * TILE_SIZE;
    r.worldHeight = DUNGEON_HEIGHT * TILE_SIZE;

    // Copy map data to Rift struct for Minimap
    for(int x=0; x<DUNGEON_WIDTH; x++) {
        for(int y=0; y<DUNGEON_HEIGHT; y++) {
            r.tileMap[x][y] = dungeonData.map[x][y];
            r.exploredMap[x][y] = false;
        }
    }
}

static Enemy MakeEnemy(EnemyType type, Vector2 pos, float hpScale, float dmgScale, int level) {
    Enemy e{};
    e.position = pos;
    e.facing = {-1, 0};
    e.radius = 18;
    e.speed = 160;
    e.maxHp = 30 * hpScale;
    e.hp = e.maxHp;
    e.damage = 10 * dmgScale;
    e.level = level;
    e.rank = Rank::F;
    e.type = type;
    e.state = EnemyState::IDLE;
    e.visionRange = 400.0f;
    e.active = true;

    switch (type) {
        case EnemyType::BRUTE:
            e.radius = 35; e.maxHp *= 4; e.hp = e.maxHp;
            e.speed = 100; e.visionRange = 500; e.damage *= 1.5f;
            e.defense = 3.0f;
            break;
        case EnemyType::SPITTER:
            e.radius = 16; e.speed = 180; e.visionRange = 550;
            e.maxHp *= 0.7f; e.hp = e.maxHp;
            break;
        case EnemyType::PHANTOM:
            e.radius = 16; e.speed = 220; e.visionRange = 350;
            e.maxHp *= 0.5f; e.hp = e.maxHp; e.damage *= 1.4f;
            break;
        case EnemyType::ELITE:
            e.radius = 30; e.maxHp *= 6; e.hp = e.maxHp;
            e.speed = 180; e.damage *= 2.5f; e.visionRange = 600;
            e.defense = 5.0f;
            break;
        case EnemyType::BOSS:
            e.radius = 50; e.maxHp *= 15; e.hp = e.maxHp;
            e.speed = 120; e.damage *= 3.0f; e.visionRange = 800;
            e.defense = 10.0f;
            break;
        default: break;
    }
    return e;
}

Rift GenerateRift(int selectedRank) {
    Rift r{};
    r.rank = (Rank)Clamp(selectedRank, 0, 6);
    r.seed = GetRandomValue(1, 999999);
    r.roomCount = 1; // Single continuous dungeon level
    r.currentRoom = 0;
    r.currentWave = 0;
    r.riftActive = true;
    r.theme = RIFT_THEMES[GetRandomValue(0, THEME_COUNT - 1)];
    
    // Build procedural dungeon
    BuildDungeon(r);
    
    return r;
}

// Populates the entire dungeon with enemies based on distance
void SpawnWave(GameWorld& world) {
    Rift& r = world.rift;
    int rankInt = (int)r.rank;
    
    // Start Zone is top-left (5,5), Boss is bottom-right (95,95)
    // We iterate the whole grid
    for (int x = 5; x < DUNGEON_WIDTH - 5; x += 3) {
        for (int y = 5; y < DUNGEON_HEIGHT - 5; y += 3) {
            
            // Skip Start Zone
            if (x < 15 && y < 15) continue;

            // Check 3x3 block for open space
            if (dungeonData.map[x][y] == 0) {
                // Determine spawn chance based on rank
                // 3% base chance per valid block (lower density for bigger map)
                if (GetRandomValue(0, 100) < 3) {
                    // Calc distance factor from start (0.0 to 1.0)
                    float dist = sqrtf((float)(x*x + y*y));
                    float maxDist = sqrtf((float)(DUNGEON_WIDTH*DUNGEON_WIDTH + DUNGEON_HEIGHT*DUNGEON_HEIGHT));
                    float distFactor = dist / maxDist; // 0.0 to 1.0
                    
                    float scale = 1.0f + (distFactor * 0.5f) + (rankInt * 0.2f);
                    int level = 1 + rankInt * 5 + (int)(distFactor * 5);
                    
                    Vector2 pos = { (float)x * TILE_SIZE + TILE_SIZE/2, (float)y * TILE_SIZE + TILE_SIZE/2 };
                    
                    // Determine enemy type
                    EnemyType t = EnemyType::SWARM;
                    int roll = GetRandomValue(1, 100);
                    
                    // Difficulty progression by distance
                    if (distFactor > 0.7f && roll < 15) t = EnemyType::BRUTE;
                    else if (distFactor > 0.4f && roll < 20) t = EnemyType::SPITTER;
                    else if (distFactor > 0.5f && roll < 15) t = EnemyType::PHANTOM;
                    else if (distFactor > 0.8f && roll < 5) t = EnemyType::ELITE;
                    
                    world.enemies.push_back(MakeEnemy(t, pos, scale, scale, level));
                }
            }
        }
    }

    // Spawn Boss at the end (Bottom Right)
    Vector2 bossPos = { (float)(DUNGEON_WIDTH - 10) * TILE_SIZE, (float)(DUNGEON_HEIGHT - 10) * TILE_SIZE };
    // Refine to valid floor
    for(int attempt=0; attempt<50; attempt++) {
         int tx = (int)(bossPos.x / TILE_SIZE);
         int ty = (int)(bossPos.y / TILE_SIZE);
         if (tx >= 0 && tx < DUNGEON_WIDTH && ty >= 0 && ty < DUNGEON_HEIGHT && dungeonData.map[tx][ty] == 0) break;
         bossPos = Vector2Add(bossPos, {(float)GetRandomValue(-300, 300), (float)GetRandomValue(-300, 300)});
    }
    
    float bossScale = 1.0f + (rankInt * 0.3f);
    int bossLevel = 5 + rankInt * 5;
    world.enemies.push_back(MakeEnemy(EnemyType::BOSS, bossPos, bossScale, bossScale, bossLevel));

    r.enemiesAlive = (int)world.enemies.size();
}

bool UpdateRiftProgress(GameWorld& world, float dt) {
    Rift& r = world.rift;
    if (!r.riftActive) return false;

    // Check specifically if BOSS is dead
    bool bossAlive = false;
    for (const auto& e : world.enemies) {
        if (e.active && e.type == EnemyType::BOSS) {
            bossAlive = true;
            break;
        }
    }
    
    // Update generic alive counter for UI (optional)
    int totalAlive = 0;
    for (const auto& e : world.enemies) if (e.active) totalAlive++;
    r.enemiesAlive = totalAlive;

    if (!bossAlive) {
        // Boss killed! Dungeon Cleared.
        world.state = GameState::RIFT_COMPLETE;
        r.riftActive = false;
        
        // Award completion rewards
        int moneyReward = 500 + (int)r.rank * 300;
        world.profile.money += moneyReward;
        AwardProfileXP(world.profile, 500 + (int)r.rank * 100);
        SaveProfile("savegame.dat", world.profile);
        return true;
    }
    
    return false;
}

void UpdateRiftFog(Rift& r, Vector2 playerPos) {
    int px = (int)(playerPos.x / TILE_SIZE);
    int py = (int)(playerPos.y / TILE_SIZE);
    int radius = 12; // Visibility radius in tiles

    for (int x = px - radius; x <= px + radius; x++) {
        for (int y = py - radius; y <= py + radius; y++) {
            if (x >= 0 && x < DUNGEON_WIDTH && y >= 0 && y < DUNGEON_HEIGHT) {
                // Circle check
                if ((x - px)*(x - px) + (y - py)*(y - py) <= radius*radius) {
                    r.exploredMap[x][y] = true;
                }
            }
        }
    }
}

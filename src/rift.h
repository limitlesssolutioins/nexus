#pragma once
#include "types.h"
#include <vector>

// Dungeon Generation Constants
// (Moved to types.h)

struct DungeonGen {
    int map[DUNGEON_WIDTH][DUNGEON_HEIGHT]; // 0 = Floor, 1 = Wall
    std::vector<Rectangle> collisionRects;
};

Rift GenerateRift(int selectedRank);
void SpawnWave(GameWorld& world);
bool UpdateRiftProgress(GameWorld& world, float dt);
void UpdateRiftFog(Rift& r, Vector2 playerPos);
bool IsWall(int x, int y);

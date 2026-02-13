#pragma once
#include "types.h"
#include "utils.h"

void DrawArena(const Rift& rift, const std::vector<Particle>& particles);
void DrawEntities(const GameWorld& world);
void DrawHUD(const GameWorld& world);
void DrawSkillTreeUI(const GameWorld& world);
void DrawMarketUI(const GameWorld& world);
void DrawHub(const GameWorld& world);
void DrawRoomClear(const Rift& rift);
void DrawRiftComplete(const Rift& rift);
void DrawGameOver(int killCount);
void DrawStatusWindow(GameWorld& world);
void DrawInventoryUI(GameWorld& world);
void DrawRiftSelectUI(const GameWorld& world);
void DrawMinimap(const Rift& rift, Vector2 playerPos);


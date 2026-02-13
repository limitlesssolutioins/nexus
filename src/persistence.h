#pragma once
#include "types.h"

Profile LoadProfile(const char* filename);
bool SaveProfile(const char* filename, const Profile& p);
Profile MakeDefaultProfile();
void AwardProfileXP(Profile& p, int xpGain);
void UpdateMissionProgress(Profile& p, int kills, EnemyType lastKilledType);
void RefreshMissions(Profile& p);
int XPForRank(int rank);
int XPForPlayerLevel(int level);

#include "persistence.h"
#include <fstream>

int XPForRank(int rank) { return 100 * (rank + 1) * (rank + 1); }

int XPForPlayerLevel(int level) { return 60 + level * 25; }

Profile MakeDefaultProfile() {
    Profile p{};
    p.saveVersion = SAVE_VERSION;
    p.hunterRankLevel = 0;
    p.exp = 0;
    p.hunterXpNext = XPForRank(0);
    p.money = 0;
    p.playerLevel = 1;
    p.playerXp = 0;
    p.playerXpNext = XPForPlayerLevel(1);
    // Starting stats: 5 in each
    p.permanentStats.STR = 5;
    p.permanentStats.AGI = 5;
    p.permanentStats.VIT = 5;
    p.permanentStats.INT = 5;
    p.permanentStats.WIS = 5;
    p.permanentStats.DEX = 5;
    p.permanentStats.SPR = 5;
    p.permanentStats.END = 5;
    p.permanentStats.unspentStatPoints = 0;
    p.permanentStats.unspentSkillPoints = 0;
    // Abilities
    for (int i = 0; i < TOTAL_ABILITIES; i++) p.unlockedAbilities[i] = false;
    for (int i = 0; i < MAX_ABILITY_SLOTS; i++) p.equippedSlots[i] = {-1, 0, 0};
    // Equipment
    for (int i = 0; i < MAX_INVENTORY; i++) p.inventory[i].active = false;
    for (int i = 0; i < EQUIP_SLOT_COUNT; i++) p.equippedIndices[i] = -1;
    
    // Tutorial not completed yet
    p.tutorialCompleted = false;

    // Default missions
    p.activeMissions[0] = {0, 50, 0, 200, 100, false};
    p.activeMissions[1] = {0, 100, 0, 500, 250, false};
    p.activeMissions[2] = {1, 15, 0, 800, 500, false};
    return p;
}

Profile LoadProfile(const char* filename) {
    std::ifstream file(filename, std::ios::binary);
    if (file.is_open()) {
        int version = 0;
        file.read(reinterpret_cast<char*>(&version), sizeof(int));
        if (version == SAVE_VERSION) {
            file.seekg(0);
            Profile p{};
            file.read(reinterpret_cast<char*>(&p), sizeof(Profile));
            file.close();
            p.hunterXpNext = XPForRank(p.hunterRankLevel);
            p.playerXpNext = XPForPlayerLevel(p.playerLevel);
            return p;
        }
        file.close();
    }
    // Old save or no save — start fresh
    return MakeDefaultProfile();
}

bool SaveProfile(const char* filename, const Profile& p) {
    std::ofstream file(filename, std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(&p), sizeof(Profile));
        file.close();
        return true;
    }
    return false;
}

void AwardProfileXP(Profile& p, int xpGain) {
    p.exp += xpGain;
    while (p.exp >= p.hunterXpNext) {
        p.exp -= p.hunterXpNext;
        p.hunterRankLevel++;
        p.hunterXpNext = XPForRank(p.hunterRankLevel);
    }
}

void UpdateMissionProgress(Profile& p, int kills, EnemyType lastType) {
    for (int i = 0; i < MAX_MISSIONS; i++) {
        if (p.activeMissions[i].completed) continue;
        bool progressed = (p.activeMissions[i].type == 0) ||
                          (p.activeMissions[i].type == 1 && lastType == EnemyType::BRUTE);
        if (progressed) {
            p.activeMissions[i].currentKills += kills;
            if (p.activeMissions[i].currentKills >= p.activeMissions[i].targetKills) {
                p.activeMissions[i].completed = true;
                AwardProfileXP(p, p.activeMissions[i].rewardXp);
                p.money += p.activeMissions[i].rewardMoney;
            }
        }
    }
}

void RefreshMissions(Profile& p) {
    for (int i = 0; i < MAX_MISSIONS; i++) {
        if (p.activeMissions[i].completed) {
            int type = GetRandomValue(0, 1);
            int target = (type == 0) ? 50 + GetRandomValue(0, 5) * 10 : 10 + GetRandomValue(0, 5);
            int xpReward = target * 10;
            int moneyReward = target * 5;
            p.activeMissions[i] = {type, target, 0, xpReward, moneyReward, false};
        }
    }
}

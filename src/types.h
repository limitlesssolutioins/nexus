#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
#include <cstring>

// --- Constants ---
constexpr int SCREEN_W = 1280;
constexpr int SCREEN_H = 720;
constexpr int MAX_ABILITY_SLOTS = 6;
constexpr int MAX_RIFT_ROOMS = 8;
constexpr float ARENA_MARGIN = 60.0f;
constexpr int DISCIPLINE_COUNT = 6;
constexpr int ABILITIES_PER_DISCIPLINE = 7;
constexpr int TOTAL_ABILITIES = DISCIPLINE_COUNT * ABILITIES_PER_DISCIPLINE; // 42
constexpr int EQUIP_SLOT_COUNT = 6;
constexpr int MAX_INVENTORY = 40;
constexpr int MAX_SUMMONS = 4;
constexpr int STAT_COUNT = 8;
constexpr int MAX_MISSIONS = 3;
constexpr int MAX_BUFFS = 8;
constexpr int SAVE_VERSION = 3;

// Leveling
constexpr int MAX_PLAYER_LEVEL = 100;
constexpr int STAT_POINTS_PER_LEVEL = 4;
constexpr int SKILL_POINTS_PER_LEVEL = 2;

// Dungeon Constants
#define TILE_SIZE 64
#define DUNGEON_WIDTH 100
#define DUNGEON_HEIGHT 100

// --- Enums ---
enum class Rank { F, E, D, C, B, A, S };

enum class Discipline { WARRIOR, MAGE, SORCERER, HUNTER, SUMMONER, HEALER, COUNT };

enum class AbilityTier { BASIC, INTERMEDIATE, ADVANCED, ULTIMATE };

enum class EnemyType { SWARM, BRUTE, SPITTER, PHANTOM, ELITE, BOSS };

enum class EnemyState { IDLE, CHASING, ATTACKING, WANDERING };

enum class EquipSlot { WEAPON, HELMET, ARMOR, BOOTS, ACCESSORY_1, ACCESSORY_2, COUNT };

enum class Rarity { COMMON, UNCOMMON, RARE, EPIC, LEGENDARY, COUNT };

enum class WeaponType { SWORD, STAFF, BOW, DAGGER, MACE, ORB, NONE, TRADE_GOOD };

enum class GameState {
    HUB, PLAYING, SKILL_TREE, RIFT_COMPLETE, GAME_OVER,
    STATUS_WINDOW, INVENTORY, MARKET, RIFT_SELECT, TUTORIAL_SELECT
};

// --- Stats ---
// Index mapping: 0=STR, 1=AGI, 2=VIT, 3=INT, 4=WIS, 5=DEX, 6=SPR, 7=END
struct Attributes {
    int STR = 0;
    int AGI = 0;
    int VIT = 0;
    int INT = 0;
    int WIS = 0;
    int DEX = 0;
    int SPR = 0;
    int END = 0;
    int unspentStatPoints = 0;
    int unspentSkillPoints = 0;

    int GetByIndex(int i) const {
        const int* arr = &STR;
        if (i >= 0 && i < STAT_COUNT) return arr[i];
        return 0;
    }
    void SetByIndex(int i, int val) {
        int* arr = &STR;
        if (i >= 0 && i < STAT_COUNT) arr[i] = val;
    }
    void AddByIndex(int i, int val) {
        int* arr = &STR;
        if (i >= 0 && i < STAT_COUNT) arr[i] += val;
    }
};

// --- Abilities ---
struct AbilityDef {
    int id;
    char name[32];
    char desc[96];
    Discipline discipline;
    AbilityTier tier;
    int skillPointCost;
    float cooldown;           // base cooldown in seconds
    Color color;
    // Stat gates
    int primaryStatIndex;     // which stat (0-7), -1 = none
    int primaryStatReq;       // minimum value required
    int secondaryStatIndex;   // -1 if none
    int secondaryStatReq;
    // Scaling
    int scalingStatIndex;     // which stat powers this ability's damage/heal
    float baseValue;          // base damage/heal/shield amount
    float scalingFactor;      // multiplied by scaling stat
};

struct AbilitySlot {
    int abilityId = -1;
    float cooldownTimer = 0;
    float cooldownMax = 0;
};

// --- Equipment ---
struct Equipment {
    char name[64] = {};
    EquipSlot slot = EquipSlot::WEAPON;
    Rarity rarity = Rarity::COMMON;
    WeaponType weaponType = WeaponType::NONE;
    int itemLevel = 1;
    float quality = 0.5f;     // 0.0 to 1.0 (determines exact stats within rank)
    float attack = 0;         // Explicit weapon damage
    float defense = 0;        // Explicit armor defense
    Attributes bonusStats = {};
    int specialEffectId = -1;
    int sellPrice = 0;
    bool active = false;      // exists in inventory
};

// --- Entities ---
struct Projectile {
    Vector2 position = {};
    Vector2 velocity = {};
    float radius = 6;
    float damage = 0;
    bool isMagic = false;     // true = scales with INT, false = scales with STR
    bool isPiercing = false;
    bool isEnemy = false;
    bool active = true;
};

struct Enemy {
    Vector2 position = {};
    Vector2 targetPos = {};
    Vector2 facing = {1, 0};
    float radius = 18;
    float speed = 160;
    float hp = 50;
    float maxHp = 50;
    float damage = 10;
    float defense = 0;
    int level = 1;
    Rank rank = Rank::F;
    EnemyType type = EnemyType::SWARM;
    EnemyState state = EnemyState::IDLE;
    float stateTimer = 0;
    float visionRange = 400;
    float attackTimer = 0;
    float specialTimer = 0;
    bool active = true;
    bool alerting = false;
    // Debuff state
    float slowTimer = 0;
    float slowFactor = 1.0f;
    float dotTimer = 0;
    float dotDamage = 0;
    float weaknessTimer = 0;
    float weaknessFactor = 1.0f;
    bool immobilized = false;
    float immobileTimer = 0;
    // AI Advanced
    float patrolTimer = 0;
    Vector2 targetPatrolPos = {};
    float alertCooldown = 0;
};

struct Summon {
    Vector2 position = {};
    Vector2 facing = {1, 0};
    float hp = 50;
    float maxHp = 50;
    float damage = 10;
    float speed = 200;
    float lifetime = 15;
    float radius = 15;
    float attackTimer = 0;
    int summonType = 0;       // 0=wisp, 1=guardian, 2=shadow
    bool active = false;
};

struct VFX {
    Vector2 position = {};
    float radius = 0;
    float maxRadius = 0;
    float lifetime = 0;
    float maxLifetime = 0;
    Color color = WHITE;
    bool active = false;
};

struct Particle {
    Vector2 position = {};
    Vector2 velocity = {};
    float lifetime = 0;
    float maxLifetime = 0;
    float size = 2;
    Color color = WHITE;
    bool active = false;
};

// --- Rift Structures ---
struct RiftTheme {
    const char* name;
    Color bgColor;
    Color accentColor;
    Color particleColor;
    Color enemyTint;
};

struct WaveDef {
    int enemyCount = 0;
    EnemyType types[4] = {};
    int typeCount = 0;
};

struct RoomDef {
    int waveCount = 0;
    WaveDef waves[6] = {};
    bool isBossRoom = false;
};

struct Obstacle {
    Rectangle rect = {};
    Color color = DARKGRAY;
    bool isHazard = false;
};

struct ResourceNode {
    Vector2 position = {};
    float hp = 30.0f;
    bool active = false;
    int value = 1; // 1=Low, 2=Med, 3=High
};

struct Decoration {
    Vector2 position = {};
    int type = 0; // 0=Rock, 1=Bones, 2=Grass, 3=Torch
    float scale = 1.0f;
};

struct Rift {
    Rank rank = Rank::F;
    int seed = 0;
    int roomCount = 3;
    int currentRoom = 0;
    int currentWave = 0;
    int enemiesAlive = 0;
    int totalKills = 0;
    float roomTransitionTimer = 0;
    bool roomCleared = false;
    bool riftActive = false;
    RiftTheme theme = {};
    RoomDef rooms[MAX_RIFT_ROOMS] = {};
    float worldWidth = SCREEN_W * 3.5f;
    float worldHeight = SCREEN_H; // Default to screen height
    std::vector<Obstacle> obstacles;
    std::vector<ResourceNode> resourceNodes;
    std::vector<Decoration> decorations;
    int tileMap[DUNGEON_WIDTH][DUNGEON_HEIGHT] = {}; // 0=Floor, 1=Wall
    bool exploredMap[DUNGEON_WIDTH][DUNGEON_HEIGHT] = {};
};

// --- Equipment Drops (replaces ImprintDrop) ---
struct EquipmentDrop {
    Vector2 position = {};
    Equipment item = {};
    float lifetime = 12.0f;
    bool active = false;
};

struct MaterialDrop {
    Vector2 position = {};
    int type = 0; // 0 = Mana Crystal, 1 = Monster Essence
    int amount = 1;
    float lifetime = 20.0f;
    bool active = false;
};

// --- Missions ---
struct Mission {
    int type = 0;            // 0 = kill count, 1 = kill specific type
    int targetKills = 50;
    int currentKills = 0;
    int rewardXp = 200;
    int rewardMoney = 100;
    bool completed = false;
};

// --- Profile (Persistent Data) ---
struct Profile {
    int saveVersion = 5; // Added materials
    // Hunter Rank
    int hunterRankLevel = 0;
    int exp = 0;
    int hunterXpNext = 100;
    int money = 0;
    
    // Materials (Currency-like)
    int manaCrystals = 0; // From mining
    int monsterEssence = 0; // From enemies
    
    bool tutorialCompleted = false;
    // Player Level (persistent)
    int playerLevel = 1;
    int playerXp = 0;
    int playerXpNext = 80;
    // Stats (persistent)
    Attributes permanentStats = {};
    // Abilities
    bool unlockedAbilities[TOTAL_ABILITIES] = {};
    AbilitySlot equippedSlots[MAX_ABILITY_SLOTS] = {};
    // Equipment
    Equipment inventory[MAX_INVENTORY] = {};
    int equippedIndices[EQUIP_SLOT_COUNT] = {-1, -1, -1, -1, -1, -1};
    // Missions
    Mission activeMissions[MAX_MISSIONS] = {};
};

// --- Player (Active Session) ---
struct Player {
    Vector2 position = {};
    float radius = 20.0f;
    float speed = 280.0f;
    float hp = 100;
    float maxHp = 100;
    float baseMaxHp = 100;
    float dmgCooldown = 0;
    float invTime = 0.5f;
    float fireTimer = 0;
    float baseFireRate = 0.15f;
    // Stats (base + equipment)
    Attributes stats = {};
    // Level (synced from profile)
    int level = 1;
    int xp = 0;
    int xpToNext = 80;
    // Buffs
    float regenTimer = 0;
    float regenAmount = 0;
    float shieldHp = 0;
    float lifestealRatio = 0;
    float damageReduction = 0;
    float buffTimers[MAX_BUFFS] = {};
    // 0=regen, 1=shield, 2=lifesteal, 3=damageReduction,
    // 4=warCry(+dmg), 5=ironSkin, 6=evasion, 7=reserved
    // Abilities
    bool unlockedAbilities[TOTAL_ABILITIES] = {};
    AbilitySlot slots[MAX_ABILITY_SLOTS] = {};
    // Summons
    Summon summons[MAX_SUMMONS] = {};
    // Computed stats (recalculated from base + equipment)
    float critChance = 0.05f;
    float critMultiplier = 1.5f;
    float physDamageMultiplier = 1.0f;
    float magicDamageMultiplier = 1.0f;
    float healPower = 1.0f;
    float summonPower = 1.0f;
    float cooldownReduction = 0;
    float attackSpeedMultiplier = 1.0f;
    float flatDefense = 0; // From armor
    // Tracking
    int killCount = 0;
    float bonusDamage = 0;
    bool miningMode = false; // Toggle between Combat and Mining
    
    // Animation
    int frame = 0;
    float frameTimer = 0.0f;
    int direction = 0; // 0: Front, 1: Side, 2: Back
    bool facingRight = true;
};

// --- Game World ---
struct GameWorld {
    Profile profile;
    Player player;
    Texture2D playerSprite;
    std::vector<Projectile> projectiles;
    std::vector<Enemy> enemies;
    std::vector<EquipmentDrop> drops;
    std::vector<MaterialDrop> materialDrops;
    std::vector<VFX> vfx;
    std::vector<Particle> particles;
    Rift rift;
    GameState state = GameState::HUB;
    // UI state
    int selectedDiscipline = 0;
    int hoveredAbility = -1;
    int selectedRiftRank = 0;
    int inventoryScroll = 0;
    Camera2D camera = {};
};

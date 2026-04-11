#pragma once
#include <QString>
#include <QMap>
#include <QSet>

// ── Item types ────────────────────────────────────────────────────────────────
// Add new items here freely — everything downstream reads from this enum
enum class ItemType {
    HealthPotion,   // restores 35% HP
    SpPotion,       // restores 50 SP
    AttackBoost,    // +50% attack for one battle
    DefenseBoost    // reduces next hit by 30%
};

// ── Building unlock flags ─────────────────────────────────────────────────────
enum class BuildingType {
    Shop,
    Lab
};

// ── Permanent stat upgrades ───────────────────────────────────────────────────
struct StatUpgrades {
    int bonusMaxHp     = 0;   // added on top of base class HP
    int bonusAttack    = 0;   // added on top of base class attack
    int bonusSpPerAtk  = 0;   // added on top of base SP-per-attack
};

// ─────────────────────────────────────────────────────────────────────────────
//  PlayerProfile
//
//  The single source of truth for everything that persists between sessions.
//  Owned by MainWindow, passed by pointer to whoever needs it.
//  Saved to profile.json whenever the player returns to the overworld.
// ─────────────────────────────────────────────────────────────────────────────
class PlayerProfile {
public:
    PlayerProfile() = default;

    // ── Identity ──────────────────────────────────────────────────────────────
    QString       characterName = "Player";
    int           characterType = 0;    // cast to CharacterType when needed

    // ── Economy ───────────────────────────────────────────────────────────────
    int           gold          = 0;

    // ── Inventory ─────────────────────────────────────────────────────────────
    // Maps each item type to how many the player owns.
    // Absent key = 0 quantity. Never goes negative.
    QMap<ItemType, int> inventory;

    // ── Progression ───────────────────────────────────────────────────────────
    QSet<BuildingType>  unlockedBuildings;
    StatUpgrades        upgrades;

    // ── Convenience helpers ───────────────────────────────────────────────────
    int  itemCount(ItemType t)       const { return inventory.value(t, 0); }
    bool hasItem(ItemType t)         const { return itemCount(t) > 0; }
    bool hasBuilding(BuildingType b) const { return unlockedBuildings.contains(b); }

    void addItem(ItemType t, int qty = 1);
    void removeItem(ItemType t, int qty = 1);   // clamps to 0, never negative
    void addGold(int amount);
    void spendGold(int amount);                  // clamps to 0
    void unlockBuilding(BuildingType b);

    // ── Persistence ───────────────────────────────────────────────────────────
    bool saveToFile(const QString &path) const;
    bool loadFromFile(const QString &path);
    void reset();                                // wipes everything back to defaults
};
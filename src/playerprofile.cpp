#include "playerprofile.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>

// ── Inventory helpers ─────────────────────────────────────────────────────────

void PlayerProfile::addItem(ItemType t, int qty)
{
    inventory[t] = inventory.value(t, 0) + qty;
}

void PlayerProfile::removeItem(ItemType t, int qty)
{
    int current = inventory.value(t, 0);
    inventory[t] = std::max(0, current - qty);
}

void PlayerProfile::addGold(int amount)
{
    gold += amount;
}

void PlayerProfile::spendGold(int amount)
{
    gold = std::max(0, gold - amount);
}

void PlayerProfile::unlockBuilding(BuildingType b)
{
    unlockedBuildings.insert(b);
}

void PlayerProfile::reset()
{
    characterName = "Player";
    characterType = 0;
    gold          = 0;
    dungeonRuns   = 0;
    inventory.clear();
    unlockedBuildings.clear();
    upgrades = StatUpgrades{};
}

// ── JSON helpers ──────────────────────────────────────────────────────────────

// ItemType ↔ string so the JSON is human-readable
static QString itemKey(ItemType t)
{
    switch (t) {
    case ItemType::HealthPotion:  return "HealthPotion";
    case ItemType::SpPotion:      return "SpPotion";
    case ItemType::AttackBoost:   return "AttackBoost";
    case ItemType::DefenseBoost:  return "DefenseBoost";
    }
    return "Unknown";
}

static ItemType itemFromKey(const QString &k)
{
    if (k == "SpPotion")     return ItemType::SpPotion;
    if (k == "AttackBoost")  return ItemType::AttackBoost;
    if (k == "DefenseBoost") return ItemType::DefenseBoost;
    return ItemType::HealthPotion;   // default / fallback
}

static QString buildingKey(BuildingType b)
{
    switch (b) {
    case BuildingType::Shop: return "Shop";
    case BuildingType::Lab:  return "Lab";
    }
    return "Unknown";
}

static BuildingType buildingFromKey(const QString &k)
{
    if (k == "Lab") return BuildingType::Lab;
    return BuildingType::Shop;
}

// ── Save ─────────────────────────────────────────────────────────────────────

bool PlayerProfile::saveToFile(const QString &path) const
{
    QJsonObject root;

    // Identity
    root["characterName"] = characterName;
    root["characterType"] = characterType;
    root["gold"]          = gold;
    root["dungeonRuns"]   = dungeonRuns;
    // Inventory — stored as { "HealthPotion": 3, "SpPotion": 1, ... }
    QJsonObject inv;
    for (auto it = inventory.constBegin(); it != inventory.constEnd(); ++it)
        if (it.value() > 0)
            inv[itemKey(it.key())] = it.value();
    root["inventory"] = inv;

    // Unlocked buildings — stored as [ "Shop", "Lab" ]
    QJsonArray buildings;
    for (BuildingType b : unlockedBuildings)
        buildings.append(buildingKey(b));
    root["unlockedBuildings"] = buildings;

    // Stat upgrades
    QJsonObject ups;
    ups["bonusMaxHp"]    = upgrades.bonusMaxHp;
    ups["bonusAttack"]   = upgrades.bonusAttack;
    ups["bonusSpPerAtk"] = upgrades.bonusSpPerAtk;
    root["upgrades"] = ups;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(root).toJson());
    return true;
}

// ── Load ─────────────────────────────────────────────────────────────────────

bool PlayerProfile::loadFromFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    if (root.isEmpty()) return false;

    characterName = root["characterName"].toString("Player");
    characterType = root["characterType"].toInt(0);
    gold          = root["gold"].toInt(0);
    dungeonRuns   = root["dungeonRuns"].toInt(0);
    // Inventory
    inventory.clear();
    QJsonObject inv = root["inventory"].toObject();
    for (auto it = inv.constBegin(); it != inv.constEnd(); ++it)
        inventory[itemFromKey(it.key())] = it.value().toInt();

    // Unlocked buildings
    unlockedBuildings.clear();
    for (const QJsonValue &v : root["unlockedBuildings"].toArray())
        unlockedBuildings.insert(buildingFromKey(v.toString()));

    // Stat upgrades
    QJsonObject ups = root["upgrades"].toObject();
    upgrades.bonusMaxHp    = ups["bonusMaxHp"].toInt(0);
    upgrades.bonusAttack   = ups["bonusAttack"].toInt(0);
    upgrades.bonusSpPerAtk = ups["bonusSpPerAtk"].toInt(0);

    return true;
}
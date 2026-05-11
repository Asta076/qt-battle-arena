#include "warrior.h"
#include <QRandomGenerator>

// ── Rebalance these freely ─────────────────────────────────────────────────
static constexpr int WARRIOR_BASE_HEALTH   = 150;
static constexpr int WARRIOR_BASE_ATTACK   = 20;
static constexpr int WARRIOR_MAX_SP        = 100;
static constexpr int WARRIOR_SP_PER_ATTACK = 15;   // slower buildup, tank class
static constexpr int WARRIOR_SPECIAL_COST  = 60;   // strong but less frequent
// ──────────────────────────────────────────────────────────────────────────

Warrior::Warrior(const QString& name)
    : Character(name,
                WARRIOR_BASE_HEALTH,
                WARRIOR_BASE_ATTACK,
                WARRIOR_MAX_SP,
                WARRIOR_SP_PER_ATTACK,
                WARRIOR_SPECIAL_COST)
{}

Warrior::~Warrior() = default;

CharacterType Warrior::getType() const { return CharacterType::Warrior; }

int Warrior::attack() const
{
    int base      = getAttackPower();
    int variation = QRandomGenerator::global()->bounded(-2, 5);
    return base + variation;
}

int Warrior::specialAbility() const
{
    // Power Strike: one heavy hit
    int base = getAttackPower() * 2;
    int variation = QRandomGenerator::global()->bounded(5, 16);
    return base + variation;
}

#include "warrior.h"
#include <QRandomGenerator>

Warrior::Warrior(const QString& name)
    : Character(name, BASE_HEALTH, BASE_ATTACK)
{}

Warrior::~Warrior() = default;

CharacterType Warrior::getType() const { return CharacterType::Warrior; }

int Warrior::attack() const
{
    // Basic swing: attackPower ± 20% variation
    int base = getAttackPower();
    int variation = QRandomGenerator::global()->bounded(-4, 5); // -4 to +4
    return base + variation;
}

int Warrior::specialAbility() const
{
    // Power Strike: 2× attack, small random bonus
    int base = getAttackPower() * 2;
    int bonus = QRandomGenerator::global()->bounded(0, 11); // 0 to 10
    return base + bonus;
}

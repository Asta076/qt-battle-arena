#include "archer.h"
#include <QRandomGenerator>

Archer::Archer(const QString& name)
    : Character(name, BASE_HEALTH, BASE_ATTACK)
{}

Archer::~Archer() = default;

CharacterType Archer::getType() const { return CharacterType::Archer; }

int Archer::attack() const
{
    // Consistent — low variance, reliable damage
    int base = getAttackPower();
    int variation = QRandomGenerator::global()->bounded(-3, 6); // -3 to +5
    return base + variation;
}

int Archer::specialAbility() const
{
    // Double Shot: two separate hits, each calculated independently
    int shot1 = getAttackPower() + QRandomGenerator::global()->bounded(0, 11);
    int shot2 = getAttackPower() + QRandomGenerator::global()->bounded(0, 11);
    return shot1 + shot2;
}

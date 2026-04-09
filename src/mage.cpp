#include "mage.h"
#include <QRandomGenerator>

Mage::Mage(const QString& name)
    : Character(name, BASE_HEALTH, BASE_ATTACK)
{}

Mage::~Mage() = default;

CharacterType Mage::getType() const { return CharacterType::Mage; }

int Mage::attack() const
{
    // Mage attacks have higher variance — sometimes fizzle, sometimes spike
    int base = getAttackPower();
    int variation = QRandomGenerator::global()->bounded(-8, 13); // -8 to +12
    return std::max(0, base + variation);
}

int Mage::specialAbility() const
{
    // Arcane Storm: massive hit but with high variance — high risk, high reward
    int base = getAttackPower() * 2;
    int storm = QRandomGenerator::global()->bounded(0, 31); // 0 to 30
    return base + storm;
}

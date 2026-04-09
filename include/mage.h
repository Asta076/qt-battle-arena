#pragma once
#include "character.h"

class Mage : public Character {
public:
    Mage(const QString& name);
    ~Mage() override;

    CharacterType getType()         const override;
    int           attack()          const override;
    int           specialAbility()  const override; // "Arcane Storm"

private:
    // Mage stats: low health, high attack
    static constexpr int BASE_HEALTH = 80;
    static constexpr int BASE_ATTACK = 35;
};

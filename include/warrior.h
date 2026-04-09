#pragma once
#include "character.h"

class Warrior : public Character {
public:
    Warrior(const QString& name);
    ~Warrior() override;

    CharacterType getType()         const override;
    int           attack()          const override;
    int           specialAbility()  const override; // "Power Strike"

private:
    // Warrior stats: high health, medium attack
    static constexpr int BASE_HEALTH  = 150;
    static constexpr int BASE_ATTACK  = 20;
};

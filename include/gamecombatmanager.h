#pragma once

#include <QPointF>
#include <QRectF>

#include "character.h"
#include "direction.h"

class GameCombatManager
{
public:
    struct AttackConfig {
        int damage = 0;
        qreal speed = 0.0;
        qreal width = 0.0;
        qreal height = 0.0;
        float lifetimeSeconds = 0.0f;
        bool piercing = false;
    };

    int maxHealthFor(CharacterType type) const;
    int meleeDamageFor(CharacterType type) const;
    int projectileDamageFor(CharacterType type) const;
    int finalDamageAfterBlock(int rawDamage, bool blocking) const;

    AttackConfig swordConfig(bool powered = false) const;
    AttackConfig arrowConfig(bool giant = false) const;
    AttackConfig fireballConfig(bool special = false) const;

    qreal projectileSpeedFor(CharacterType type) const;
    int projectileMaxTicksFor(CharacterType type) const;

    QPointF directionVector(Direction dir) const;
    QRectF meleeHitBox(const QPointF& actorPos,
                       qreal actorW,
                       qreal actorH,
                       Direction facing) const;

    CharacterType projectileTypeFor(CharacterType attacker) const;
    bool usesProjectile(CharacterType attacker) const;

    static constexpr int BLOCK_DAMAGE_PERCENT = 30;
};

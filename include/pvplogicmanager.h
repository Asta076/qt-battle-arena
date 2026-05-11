#pragma once

#include <QPointF>
#include <QRectF>
#include <QSet>

#include "character.h"
#include "direction.h"
#include "gamecombatmanager.h"
#include "gameplaymovementmanager.h"

// Central source for PvP combat/movement numbers.
// PvpArenaWidget should render state and forward input only.
class PvpLogicManager
{
public:
    Direction directionFromVelocity(const QPointF& velocity, Direction fallback) const;

    QPointF playerVelocity(const QSet<int>& heldKeys,
                           int playerNumber,
                           qreal baseSpeed,
                           bool blocking) const;

    QPointF clampToArena(const QPointF& pos,
                         qreal actorW,
                         qreal actorH,
                         const QRectF& arenaBounds) const;

    int maxHpFor(CharacterType type) const;
    int projectileDamageFor(CharacterType type) const;
    int meleeDamageFor(CharacterType type) const;
    int damageAfterBlock(int rawDamage, bool blocking) const;

    qreal projectileSpeed() const;
    int projectileMaxTicks() const;
    qreal knockbackDistance() const;
    int hitFlashTicks() const;

    QPointF directionVector(Direction dir) const;

private:
    GameplayMovementManager m_movement;
    GameCombatManager m_combat;

    static constexpr qreal BLOCK_SPEED_MULTIPLIER = 0.45;
    static constexpr qreal PROJECTILE_SPEED = 9.0;
    static constexpr int PROJECTILE_MAX_TICKS = 90;
    static constexpr qreal KNOCKBACK_DISTANCE = 28.0;
    static constexpr int HIT_FLASH_TICKS = 8;
};

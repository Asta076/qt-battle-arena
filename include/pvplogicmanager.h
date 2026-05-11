#pragma once

#include <QPointF>
#include <QRectF>
#include <QSet>

#include "character.h"
#include "direction.h"
#include "gamecombatmanager.h"
#include "gameplaymovementmanager.h"

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
    QPointF directionVector(Direction dir) const;

private:
    GameplayMovementManager m_movement;
    GameCombatManager m_combat;

    static constexpr qreal BLOCK_SPEED_MULTIPLIER = 0.45;
};

#pragma once

#include <QList>
#include <QPointF>
#include <QRectF>
#include <QSet>

#include "direction.h"
#include "gameplaymovementmanager.h"

class OverworldLogicManager
{
public:
    enum class Trigger {
        None,
        Dungeon,
        House,
        Shop,
        Levels
    };

    struct MovementResult {
        QPointF position;
        bool moving = false;
        Direction facing = Direction::Down;
    };

    MovementResult resolvePlayerMovement(const QPointF& currentPos,
                                         const QSet<int>& heldKeys,
                                         qreal speed,
                                         qreal playerW,
                                         qreal playerH,
                                         const QRectF& worldBounds,
                                         const QList<QRectF>& solidRects,
                                         Direction fallbackFacing) const;

    Trigger triggerFor(const QRectF& playerRect,
                       const QRectF& dungeonRect,
                       const QRectF& houseEntranceRect,
                       const QRectF& shopRect,
                       const QRectF& levelsRect,
                       const QSet<int>& heldKeys) const;

private:
    GameplayMovementManager m_movement;
};

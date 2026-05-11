#pragma once

#include <QList>
#include <QPointF>
#include <QRectF>
#include <QSet>

#include "direction.h"

class GameplayMovementManager
{
public:
    enum class ControlScheme {
        Wasd,
        Arrows,
        WasdAndArrows
    };

    QPointF velocityFromKeys(const QSet<int>& heldKeys,
                             qreal speed,
                             ControlScheme scheme = ControlScheme::WasdAndArrows,
                             qreal speedMultiplier = 1.0) const;

    Direction directionFromVelocity(const QPointF& velocity,
                                    Direction fallback = Direction::Down) const;

    Direction directionFromKeys(const QSet<int>& heldKeys,
                                ControlScheme scheme = ControlScheme::WasdAndArrows,
                                Direction fallback = Direction::Down) const;

    bool isMoving(const QSet<int>& heldKeys,
                  ControlScheme scheme = ControlScheme::WasdAndArrows) const;

    QPointF clampToBounds(const QPointF& proposed,
                          qreal objectW,
                          qreal objectH,
                          const QRectF& bounds) const;

    // Reject-style movement: try X, then Y. This prevents collision boxes from
    // teleporting the player to an edge.
    QPointF resolveMovement(const QPointF& currentPos,
                            const QPointF& velocity,
                            qreal objectW,
                            qreal objectH,
                            const QRectF& bounds,
                            const QList<QRectF>& solidRects = {}) const;

private:
    bool blockedAt(const QPointF& pos,
                   qreal objectW,
                   qreal objectH,
                   const QList<QRectF>& solidRects) const;
};

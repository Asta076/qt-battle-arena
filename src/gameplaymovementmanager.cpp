#include "gameplaymovementmanager.h"

#include <algorithm>
#include <cmath>

#include <Qt>

QPointF GameplayMovementManager::velocityFromKeys(const QSet<int>& heldKeys,
                                                  qreal speed,
                                                  ControlScheme scheme,
                                                  qreal speedMultiplier) const
{
    qreal dx = 0.0;
    qreal dy = 0.0;
    const qreal finalSpeed = speed * speedMultiplier;

    const bool useWasd = scheme == ControlScheme::Wasd || scheme == ControlScheme::WasdAndArrows;
    const bool useArrows = scheme == ControlScheme::Arrows || scheme == ControlScheme::WasdAndArrows;

    if (useWasd) {
        if (heldKeys.contains(Qt::Key_W)) dy -= finalSpeed;
        if (heldKeys.contains(Qt::Key_S)) dy += finalSpeed;
        if (heldKeys.contains(Qt::Key_A)) dx -= finalSpeed;
        if (heldKeys.contains(Qt::Key_D)) dx += finalSpeed;
    }

    if (useArrows) {
        if (heldKeys.contains(Qt::Key_Up)) dy -= finalSpeed;
        if (heldKeys.contains(Qt::Key_Down)) dy += finalSpeed;
        if (heldKeys.contains(Qt::Key_Left)) dx -= finalSpeed;
        if (heldKeys.contains(Qt::Key_Right)) dx += finalSpeed;
    }

    if (dx != 0.0 && dy != 0.0) {
        dx *= 0.70710678;
        dy *= 0.70710678;
    }

    return QPointF(dx, dy);
}

Direction GameplayMovementManager::directionFromVelocity(const QPointF& velocity,
                                                         Direction fallback) const
{
    const qreal dx = velocity.x();
    const qreal dy = velocity.y();

    if (std::abs(dx) < 0.001 && std::abs(dy) < 0.001)
        return fallback;

    if (dx > 0.0 && dy < 0.0) return Direction::ForwardRight;
    if (dx < 0.0 && dy < 0.0) return Direction::ForwardLeft;
    if (dx > 0.0 && dy > 0.0) return Direction::DownRight;
    if (dx < 0.0 && dy > 0.0) return Direction::DownLeft;

    if (dx > 0.0) return Direction::Right;
    if (dx < 0.0) return Direction::Left;
    if (dy < 0.0) return Direction::Up;
    if (dy > 0.0) return Direction::Down;

    return fallback;
}

Direction GameplayMovementManager::directionFromKeys(const QSet<int>& heldKeys,
                                                     ControlScheme scheme,
                                                     Direction fallback) const
{
    return directionFromVelocity(velocityFromKeys(heldKeys, 1.0, scheme), fallback);
}

bool GameplayMovementManager::isMoving(const QSet<int>& heldKeys, ControlScheme scheme) const
{
    QPointF velocity = velocityFromKeys(heldKeys, 1.0, scheme);
    return std::abs(velocity.x()) > 0.001 || std::abs(velocity.y()) > 0.001;
}

QPointF GameplayMovementManager::clampToBounds(const QPointF& proposed,
                                               qreal objectW,
                                               qreal objectH,
                                               const QRectF& bounds) const
{
    return QPointF(
        std::clamp(proposed.x(), bounds.left(), bounds.right() - objectW),
        std::clamp(proposed.y(), bounds.top(), bounds.bottom() - objectH)
    );
}

bool GameplayMovementManager::blockedAt(const QPointF& pos,
                                        qreal objectW,
                                        qreal objectH,
                                        const QList<QRectF>& solidRects) const
{
    QRectF actorRect(pos.x(), pos.y(), objectW, objectH);

    for (const QRectF& solid : solidRects) {
        if (solid.isValid() && actorRect.intersects(solid))
            return true;
    }

    return false;
}

QPointF GameplayMovementManager::resolveMovement(const QPointF& currentPos,
                                                 const QPointF& velocity,
                                                 qreal objectW,
                                                 qreal objectH,
                                                 const QRectF& bounds,
                                                 const QList<QRectF>& solidRects) const
{
    QPointF nextPos = currentPos;

    QPointF xAttempt = nextPos;
    xAttempt.setX(std::clamp(currentPos.x() + velocity.x(),
                             bounds.left(),
                             bounds.right() - objectW));

    if (!blockedAt(xAttempt, objectW, objectH, solidRects))
        nextPos.setX(xAttempt.x());

    QPointF yAttempt = nextPos;
    yAttempt.setY(std::clamp(currentPos.y() + velocity.y(),
                             bounds.top(),
                             bounds.bottom() - objectH));

    if (!blockedAt(yAttempt, objectW, objectH, solidRects))
        nextPos.setY(yAttempt.y());

    return nextPos;
}

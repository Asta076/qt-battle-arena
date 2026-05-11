#include "overworldlogicmanager.h"

#include <Qt>

OverworldLogicManager::MovementResult OverworldLogicManager::resolvePlayerMovement(
    const QPointF& currentPos,
    const QSet<int>& heldKeys,
    qreal speed,
    qreal playerW,
    qreal playerH,
    const QRectF& worldBounds,
    const QList<QRectF>& solidRects,
    Direction fallbackFacing) const
{
    QPointF velocity = m_movement.velocityFromKeys(
        heldKeys,
        speed,
        GameplayMovementManager::ControlScheme::WasdAndArrows
    );

    MovementResult result;
    result.position = m_movement.resolveMovement(currentPos,
                                                 velocity,
                                                 playerW,
                                                 playerH,
                                                 worldBounds,
                                                 solidRects);
    result.moving = velocity.x() != 0.0 || velocity.y() != 0.0;
    result.facing = m_movement.directionFromVelocity(velocity, fallbackFacing);
    return result;
}

OverworldLogicManager::Trigger OverworldLogicManager::triggerFor(
    const QRectF& playerRect,
    const QRectF& dungeonRect,
    const QRectF& houseEntranceRect,
    const QRectF& shopRect,
    const QRectF& levelsRect,
    const QSet<int>& heldKeys) const
{
    if (dungeonRect.isValid() && playerRect.intersects(dungeonRect))
        return Trigger::Dungeon;

    if (houseEntranceRect.isValid() && playerRect.intersects(houseEntranceRect)) {
        const bool movingUp = heldKeys.contains(Qt::Key_W) || heldKeys.contains(Qt::Key_Up);
        if (movingUp)
            return Trigger::House;
    }

    if (shopRect.isValid() && playerRect.intersects(shopRect))
        return Trigger::Shop;

    if (levelsRect.isValid() && playerRect.intersects(levelsRect))
        return Trigger::Levels;

    return Trigger::None;
}

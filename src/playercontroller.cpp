#include "playercontroller.h"

#include "gameplaymovementmanager.h"

QPointF PlayerController::computeVelocity(const QSet<int>& heldKeys) const
{
    GameplayMovementManager movement;
    return movement.velocityFromKeys(
        heldKeys,
        m_speed,
        GameplayMovementManager::ControlScheme::WasdAndArrows
    );
}

QPointF PlayerController::clampToWorld(QPointF proposed,
                                       qreal objectW,
                                       qreal objectH,
                                       qreal worldW,
                                       qreal worldH) const
{
    GameplayMovementManager movement;
    return movement.clampToBounds(
        proposed,
        objectW,
        objectH,
        QRectF(0, 0, worldW, worldH)
    );
}

Direction PlayerController::computeDirection(const QSet<int>& heldKeys) const
{
    GameplayMovementManager movement;
    return movement.directionFromKeys(
        heldKeys,
        GameplayMovementManager::ControlScheme::WasdAndArrows,
        Direction::Down
    );
}

bool PlayerController::isMoving(const QSet<int>& heldKeys) const
{
    GameplayMovementManager movement;
    return movement.isMoving(
        heldKeys,
        GameplayMovementManager::ControlScheme::WasdAndArrows
    );
}

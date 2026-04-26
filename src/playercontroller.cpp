#include "playercontroller.h"
#include <QKeyEvent>
#include <QtMath>

QPointF PlayerController::computeVelocity(const QSet<int>& heldKeys) const
{
    const bool goUp    = heldKeys.contains(Qt::Key_W) || heldKeys.contains(Qt::Key_Up);
    const bool goDown  = heldKeys.contains(Qt::Key_S) || heldKeys.contains(Qt::Key_Down);
    const bool goLeft  = heldKeys.contains(Qt::Key_A) || heldKeys.contains(Qt::Key_Left);
    const bool goRight = heldKeys.contains(Qt::Key_D) || heldKeys.contains(Qt::Key_Right);

    qreal dx = 0, dy = 0;
    if (goUp)    dy -= m_speed;
    if (goDown)  dy += m_speed;
    if (goLeft)  dx -= m_speed;
    if (goRight) dx += m_speed;

    // Normalize diagonal so speed stays consistent
    if (dx != 0 && dy != 0) {
        dx *= 0.7071;
        dy *= 0.7071;
    }

    return QPointF(dx, dy);
}

QPointF PlayerController::clampToWorld(QPointF proposed,
                                       qreal objectW, qreal objectH,
                                       qreal worldW,  qreal worldH) const
{
    qreal x = qBound(0.0,      proposed.x(), worldW - objectW);
    qreal y = qBound(0.0,      proposed.y(), worldH - objectH);
    return QPointF(x, y);
}

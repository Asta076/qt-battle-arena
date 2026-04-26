#pragma once
#include <QPointF>
#include <QSet>


//  PlayerController
//  Used by both OverworldWidget and DungeonWidget.

class PlayerController {
public:
    PlayerController() = default;

    // Call every tick with currently held keys.
    // Returns normalized velocity vector (dx, dy), scaled by m_speed.
    QPointF computeVelocity(const QSet<int>& heldKeys) const;

    // Clamp a proposed position inside world bounds.
    // objectW/H = size of the moving object.
    QPointF clampToWorld(QPointF proposed,
                         qreal objectW, qreal objectH,
                         qreal worldW,  qreal worldH) const;

    void setSpeed(qreal speed) { m_speed = speed; }
    qreal speed() const        { return m_speed;  }

private:
    qreal m_speed = 3.0;
};

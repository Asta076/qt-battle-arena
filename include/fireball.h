#pragma once

#include "projectile.h"

class Fireball : public Projectile {
public:
    Fireball(const QRectF& playerBounds,
             Direction facing,
             qreal size,
             qreal speed,
             int damage,
             float lifetime);

    void attachToScene(QGraphicsScene* scene, Direction facing) override;
};

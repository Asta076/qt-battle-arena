#pragma once

#include "projectile.h"

class Arrow : public Projectile {
public:
    Arrow(const QRectF& playerBounds,
          Direction facing,
          qreal width,
          qreal height,
          qreal speed,
          int damage,
          float lifetime,
          bool piercing);

    void attachToScene(QGraphicsScene* scene, Direction facing) override;
};

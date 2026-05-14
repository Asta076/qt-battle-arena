#include "fireball.h"

#include <QBrush>
#include <QColor>
#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QPen>
#include <QPixmap>

Fireball::Fireball(const QRectF& playerBounds,
                   Direction facing,
                   qreal size,
                   qreal speed,
                   int damage,
                   float lifetime)
    : Projectile(
          AttackType::Fireball,
          QRectF(playerBounds.center().x() - size / 2.0,
                 playerBounds.center().y() - size / 2.0,
                 size,
                 size),
          Projectile::directionVector(facing) * speed,
          damage,
          lifetime,
          false)
{
}

void Fireball::attachToScene(QGraphicsScene* scene, Direction facing)
{
    if (!scene)
        return;

    QPixmap px(":/sprites/fireball.png");

    if (!px.isNull()) {
        auto* item = new QGraphicsPixmapItem(
            px.scaled(static_cast<int>(bounds.width()),
                      static_cast<int>(bounds.height()),
                      Qt::KeepAspectRatio,
                      Qt::FastTransformation)
        );

        item->setTransformOriginPoint(item->boundingRect().center());
        item->setRotation(Projectile::rotationFor(facing));
        item->setPos(bounds.topLeft());
        item->setZValue(10);

        scene->addItem(item);
        visual = item;
        return;
    }

    auto* item = scene->addEllipse(
        bounds,
        QPen(QColor("#ff7043")),
        QBrush(QColor("#ff7043"))
    );

    item->setZValue(10);
    visual = item;
}

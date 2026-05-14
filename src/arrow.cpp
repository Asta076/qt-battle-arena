#include "arrow.h"

#include <QBrush>
#include <QColor>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QPen>
#include <QPixmap>

Arrow::Arrow(const QRectF& playerBounds,
             Direction facing,
             qreal width,
             qreal height,
             qreal speed,
             int damage,
             float lifetime,
             bool piercing)
    : Projectile(
          AttackType::Arrow,
          QRectF(playerBounds.center().x() - width / 2.0,
                 playerBounds.center().y() - height / 2.0,
                 width,
                 height),
          Projectile::directionVector(facing) * speed,
          damage,
          lifetime,
          piercing)
{
}

void Arrow::attachToScene(QGraphicsScene* scene, Direction facing)
{
    if (!scene)
        return;

    QPixmap px(":/sprites/arrow.png");

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

    auto* item = scene->addRect(
        bounds,
        QPen(QColor("#c8a15a")),
        QBrush(QColor("#c8a15a"))
    );

    item->setZValue(10);
    visual = item;
}

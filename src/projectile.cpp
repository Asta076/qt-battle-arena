#include "projectile.h"

#include <algorithm>

Projectile::Projectile(AttackType attackType,
                       const QRectF& attackBounds,
                       const QPointF& attackVelocity,
                       int attackDamage,
                       float attackLifetime,
                       bool attackPiercing)
    : type(attackType),
      bounds(attackBounds),
      velocity(attackVelocity),
      damage(attackDamage),
      lifetime(attackLifetime),
      piercing(attackPiercing)
{
}

Projectile::~Projectile()
{
    delete visual;
    visual = nullptr;
}

void Projectile::update(float deltaTime)
{
    if (expired)
        return;

    lifetime -= deltaTime;

    if (lifetime <= 0.0f) {
        expire();
        return;
    }

    const QPointF movement = velocity * deltaTime;
    bounds.translate(movement);

    if (visual)
        visual->moveBy(movement.x(), movement.y());
}

void Projectile::attachToScene(QGraphicsScene*, Direction)
{
    // Base projectile has no default visual.
    // Arrow and Fireball override this.
}

void Projectile::expire()
{
    expired = true;
}

QPointF Projectile::directionVector(Direction facing)
{
    static constexpr qreal D = 0.70710678;

    switch (facing) {
    case Direction::Right:        return { 1.0,  0.0};
    case Direction::ForwardRight: return { D,   -D};
    case Direction::Up:           return { 0.0, -1.0};
    case Direction::ForwardLeft:  return {-D,   -D};
    case Direction::Left:         return {-1.0,  0.0};
    case Direction::DownLeft:     return {-D,    D};
    case Direction::Down:         return { 0.0,  1.0};
    case Direction::DownRight:    return { D,    D};
    }

    return {0.0, 1.0};
}

qreal Projectile::rotationFor(Direction facing)
{
    switch (facing) {
    case Direction::Right:        return 0.0;
    case Direction::ForwardRight: return -45.0;
    case Direction::Up:           return -90.0;
    case Direction::ForwardLeft:  return -135.0;
    case Direction::Left:         return 180.0;
    case Direction::DownLeft:     return 135.0;
    case Direction::Down:         return 90.0;
    case Direction::DownRight:    return 45.0;
    }

    return 0.0;
}

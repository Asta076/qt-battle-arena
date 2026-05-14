#pragma once

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QPointF>
#include <QRectF>
#include <QSet>

#include "direction.h"

class Enemy;

enum class AttackType {
    Sword,
    Arrow,
    Fireball
};

// Base attack/projectile model used by WorldCombatManager, DungeonWidget, and Level1Widget.
// The public fields intentionally match the old ActiveAttack struct, so existing collision
// code can keep using attack->bounds, attack->damage, attack->piercing, etc.
class Projectile {
public:
    Projectile(AttackType type,
               const QRectF& bounds,
               const QPointF& velocity,
               int damage,
               float lifetime,
               bool piercing);

    virtual ~Projectile();

    virtual void update(float deltaTime);
    virtual void attachToScene(QGraphicsScene* scene, Direction facing);

    void expire();

    static QPointF directionVector(Direction facing);
    static qreal rotationFor(Direction facing);

    AttackType type;
    QRectF bounds;
    QPointF velocity;
    int damage = 0;
    float lifetime = 0.0f;
    bool expired = false;
    bool piercing = false;

    // Used by piercing attacks so they do not damage the same enemy every frame.
    QSet<Enemy*> enemiesHit;

    // Owned by this Projectile. WorldCombatManager removes it from the scene before deleting
    // the projectile; the destructor deletes the item itself.
    QGraphicsItem* visual = nullptr;
};

// Compatibility alias: the rest of the game can keep using ActiveAttack*.
using ActiveAttack = Projectile;

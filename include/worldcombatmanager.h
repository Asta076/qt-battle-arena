#pragma once

#include <QObject>
#include <QHash>
#include <QList>
#include <QRectF>
#include <QPointF>
#include <QGraphicsScene>
#include <QGraphicsItem>

#include "character.h"
#include "enemy.h"
#include "direction.h"

enum class AttackType {
    Sword,
    Arrow,
    Fireball
};

struct ActiveAttack {
    AttackType type;
    QRectF bounds;
    QPointF velocity;
    int damage = 0;
    float lifetime = 0.0f;
    bool expired = false;
    QGraphicsItem* visual = nullptr;
};

class WorldCombatManager : public QObject
{
    Q_OBJECT

public:
    explicit WorldCombatManager(QObject* parent = nullptr);

    void setPlayer(Character* player);
    void setScene(QGraphicsScene* scene);

    void registerEnemy(Enemy* enemy);
    void unregisterEnemy(Enemy* enemy);
    void clearEnemies();

    void update(float deltaTime);

    bool canPlayerSwordAttack() const;
    bool canPlayerShoot() const;
    bool canEnemyAttack(Enemy* enemy) const;

    ActiveAttack* createSwordSwing(const QRectF& playerBounds, Direction facing);
    ActiveAttack* shootArrow(const QRectF& playerBounds, Direction facing);
    ActiveAttack* shootFireball(const QRectF& playerBounds, Direction facing);

    const QList<ActiveAttack*>& activeAttacks() const;

    void expireAttack(ActiveAttack* attack);

    int enemyAttackDamage(Enemy* enemy) const;

    void damageEnemy(Enemy* enemy, int amount);
    void damagePlayer(int amount);

    void startEnemyAttackCooldown(Enemy* enemy);

    bool isPlayerAlive() const;

signals:
    void enemyDamaged(Enemy* enemy, int damage);
    void enemyDied(Enemy* enemy);

    void playerDamaged(int damage);
    void playerDied();

private:
    Character* m_player = nullptr;
    QGraphicsScene* m_scene = nullptr;

    QHash<Enemy*, float> m_enemyAttackCooldowns;
    QList<ActiveAttack*> m_activeAttacks;

    QPointF directionVector(Direction facing) const;
    QRectF swordBounds(const QRectF& playerBounds, Direction facing) const;

    void updateCooldowns(float deltaTime);
    void updateAttacks(float deltaTime);
    void removeExpiredAttacks();
};

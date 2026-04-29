#pragma once

#include <QObject>
#include <QHash>

#include "character.h"
#include "enemy.h"

class WorldCombatManager : public QObject
{
    Q_OBJECT

public:
    explicit WorldCombatManager(QObject* parent = nullptr);

    void setPlayer(Character* player);

    void registerEnemy(Enemy* enemy);
    void unregisterEnemy(Enemy* enemy);
    void clearEnemies();

    void update(float deltaTime);

    bool canPlayerSwordAttack() const;
    bool canPlayerShoot() const;
    bool canEnemyAttack(Enemy* enemy) const;

    int playerSwordDamage();
    int playerProjectileDamage();
    int enemyAttackDamage(Enemy* enemy) const;

    void damageEnemy(Enemy* enemy, int amount);
    void damagePlayer(int amount);

    void startPlayerSwordCooldown();
    void startPlayerShootCooldown();
    void startEnemyAttackCooldown(Enemy* enemy);

    bool isPlayerAlive() const;

signals:
    void enemyDamaged(Enemy* enemy, int damage);
    void enemyDied(Enemy* enemy);

    void playerDamaged(int damage);
    void playerDied();

private:
    Character* m_player = nullptr;

    QHash<Enemy*, float> m_enemyAttackCooldowns;

    float m_playerSwordCooldown = 0.0f;
    float m_playerShootCooldown = 0.0f;

    static constexpr float PLAYER_SWORD_COOLDOWN = 0.35f;
    static constexpr float PLAYER_SHOOT_COOLDOWN = 0.45f;
    static constexpr float ENEMY_ATTACK_COOLDOWN = 0.80f;

    static constexpr int PLAYER_SWORD_BASE_DAMAGE = 25;
    static constexpr int PLAYER_PROJECTILE_BASE_DAMAGE = 18;
};

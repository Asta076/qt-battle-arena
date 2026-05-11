#pragma once

#include <QObject>
#include <QHash>
#include <QList>
#include <QRectF>
#include <QPointF>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QSet>

#include "character.h"
#include "enemy.h"
#include "direction.h"
#include "gamecombatmanager.h"

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

    // If true, this attack keeps flying after hitting an enemy.
    bool piercing = false;

    // Prevents piercing attacks from damaging the same enemy every frame.
    QSet<Enemy*> enemiesHit;

    QGraphicsItem* visual = nullptr;
};

class WorldCombatManager : public QObject
{
    Q_OBJECT

public:
    explicit WorldCombatManager(QObject* parent = nullptr);
    ~WorldCombatManager() override;

    void setPlayer(Character* player);
    Character* player() const { return m_player; }

    void setScene(QGraphicsScene* scene);

    void registerEnemy(Enemy* enemy);
    void unregisterEnemy(Enemy* enemy);
    void clearEnemies();

    void update(float deltaTime);

    bool canPlayerSwordAttack() const;
    bool canPlayerShoot() const;
    bool canEnemyAttack(Enemy* enemy) const;

    ActiveAttack* createPlayerAttack(const QRectF& playerBounds, Direction facing);
    QList<ActiveAttack*> createPlayerSpecial(const QRectF& playerBounds, Direction facing);

    ActiveAttack* createSwordSwing(const QRectF& playerBounds, Direction facing);
    ActiveAttack* shootArrow(const QRectF& playerBounds, Direction facing);
    ActiveAttack* shootFireball(const QRectF& playerBounds, Direction facing);

    const QList<ActiveAttack*>& activeAttacks() const;

    void expireAttack(ActiveAttack* attack);
    void clearAttacks();

    int enemyAttackDamage(Enemy* enemy) const;

    void damageEnemy(Enemy* enemy, int amount);
    void damagePlayer(int amount);

    void startEnemyAttackCooldown(Enemy* enemy);

    bool isPlayerAlive() const;

    float specialMeter() const { return m_specialMeter; }
    void addSpecialFromKill();
    void resetSpecialMeter();

    bool warriorSpecialActive() const { return m_warriorSpecialActive; }

signals:
    void enemyDamaged(Enemy* enemy, int damage);
    void enemyDied(Enemy* enemy);

    void playerDamaged(int damage);
    void playerDied();

private:
    GameCombatManager m_sharedCombat;

    Character* m_player = nullptr;
    QGraphicsScene* m_scene = nullptr;

    QHash<Enemy*, float> m_enemyAttackCooldowns;
    QList<ActiveAttack*> m_activeAttacks;

    float m_playerSwordCooldown = 0.0f;
    float m_playerShootCooldown = 0.0f;

    float m_specialMeter = 0.0f;
    bool m_warriorSpecialActive = false;

    static constexpr float PLAYER_SWORD_COOLDOWN = 0.35f;
    static constexpr float PLAYER_SHOOT_COOLDOWN = 1.0f;
    static constexpr float ENEMY_ATTACK_COOLDOWN = 1.0f;

    static constexpr float SPECIAL_GAIN_PER_KILL = 0.10f;
    static constexpr float MAGE_SPECIAL_COST = 1.0f / 3.0f;
    static constexpr float ARCHER_SPECIAL_COST = 0.20f;
    static constexpr float WARRIOR_SPECIAL_DRAIN_PER_SECOND = 0.20f;

    static constexpr int SWORD_DAMAGE = 25;
    static constexpr int ARROW_DAMAGE = 18;
    static constexpr int FIREBALL_DAMAGE = 30;

    static constexpr int GIANT_ARROW_DAMAGE = 45;
    static constexpr int MAGE_SPECIAL_FIREBALL_DAMAGE = 24;

    QPointF directionVector(Direction facing) const;
    QRectF swordBounds(const QRectF& playerBounds, Direction facing) const;

    void updateCooldowns(float deltaTime);
    void updateSpecial(float deltaTime);
    void updateAttacks(float deltaTime);
    void removeExpiredAttacks();

    int applyPlayerDamageModifiers(int baseDamage) const;

    ActiveAttack* createArrowProjectile(const QRectF& playerBounds,
                                        Direction facing,
                                        qreal width,
                                        qreal height,
                                        qreal speed,
                                        int damage,
                                        float lifetime,
                                        bool startCooldown,
                                        bool piercing);

    ActiveAttack* createFireballProjectile(const QRectF& playerBounds,
                                           Direction facing,
                                           qreal size,
                                           qreal speed,
                                           int damage,
                                           float lifetime,
                                           bool startCooldown);
};

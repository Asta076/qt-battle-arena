#include "worldcombatmanager.h"

#include <algorithm>

#include <QBrush>
#include <QColor>
#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QPen>
#include <QPixmap>
#include <QTransform>


WorldCombatManager::WorldCombatManager(QObject* parent)
    : QObject(parent)
{
}

WorldCombatManager::~WorldCombatManager()
{
    clearAttacks();
}

void WorldCombatManager::setPlayer(Character* player)
{
    m_player = player;
}

void WorldCombatManager::setScene(QGraphicsScene* scene)
{
    m_scene = scene;
}

void WorldCombatManager::registerEnemy(Enemy* enemy)
{
    if (!enemy)
        return;

    if (!m_enemyAttackCooldowns.contains(enemy))
        m_enemyAttackCooldowns.insert(enemy, 0.0f);
}

void WorldCombatManager::unregisterEnemy(Enemy* enemy)
{
    if (!enemy)
        return;

    m_enemyAttackCooldowns.remove(enemy);
}

void WorldCombatManager::clearEnemies()
{
    m_enemyAttackCooldowns.clear();
}

void WorldCombatManager::update(float deltaTime)
{
    updateCooldowns(deltaTime);
    updateAttacks(deltaTime);
    removeExpiredAttacks();
}

void WorldCombatManager::updateCooldowns(float deltaTime)
{
    m_playerSwordCooldown = std::max(0.0f, m_playerSwordCooldown - deltaTime);
    m_playerShootCooldown = std::max(0.0f, m_playerShootCooldown - deltaTime);

    for (auto it = m_enemyAttackCooldowns.begin(); it != m_enemyAttackCooldowns.end(); ++it) {
        it.value() = std::max(0.0f, it.value() - deltaTime);
    }
}

void WorldCombatManager::updateAttacks(float deltaTime)
{
    for (ActiveAttack* attack : m_activeAttacks) {
        if (!attack || attack->expired)
            continue;

        attack->lifetime -= deltaTime;

        if (attack->lifetime <= 0.0f) {
            attack->expired = true;
            continue;
        }

        QPointF movement = attack->velocity * deltaTime;
        attack->bounds.translate(movement);

        if (attack->visual)
            attack->visual->moveBy(movement.x(), movement.y());
    }
}

void WorldCombatManager::removeExpiredAttacks()
{
    for (int i = m_activeAttacks.size() - 1; i >= 0; --i) {
        ActiveAttack* attack = m_activeAttacks[i];

        if (!attack || !attack->expired)
            continue;

        if (attack->visual && m_scene)
            m_scene->removeItem(attack->visual);

        delete attack->visual;
        delete attack;

        m_activeAttacks.removeAt(i);
    }
}

void WorldCombatManager::clearAttacks()
{
    for (ActiveAttack* attack : m_activeAttacks) {
        if (!attack)
            continue;

        if (attack->visual && m_scene)
            m_scene->removeItem(attack->visual);

        delete attack->visual;
        delete attack;
    }

    m_activeAttacks.clear();
}

bool WorldCombatManager::canPlayerSwordAttack() const
{
    return m_player && m_player->isAlive() && m_playerSwordCooldown <= 0.0f;
}

bool WorldCombatManager::canPlayerShoot() const
{
    return m_player && m_player->isAlive() && m_playerShootCooldown <= 0.0f;
}

bool WorldCombatManager::canEnemyAttack(Enemy* enemy) const
{
    if (!enemy || !enemy->isAlive())
        return false;

    return m_enemyAttackCooldowns.value(enemy, 0.0f) <= 0.0f;
}

ActiveAttack* WorldCombatManager::createPlayerAttack(const QRectF& playerBounds, Direction facing)
{
    if (!m_player || !m_player->isAlive())
        return nullptr;

    switch (m_player->getType()) {
    case CharacterType::Warrior:
        return createSwordSwing(playerBounds, facing);

    case CharacterType::Archer:
        return shootArrow(playerBounds, facing);

    case CharacterType::Mage:
        return shootFireball(playerBounds, facing);
    }

    return nullptr;
}

QPointF WorldCombatManager::directionVector(Direction facing) const
{
    static constexpr qreal DIAGONAL = 0.70710678;

    switch (facing) {
    case Direction::Right:
        return {1.0, 0.0};

    case Direction::Left:
        return {-1.0, 0.0};

    case Direction::Up:
        return {0.0, -1.0};

    case Direction::Down:
        return {0.0, 1.0};

    case Direction::ForwardRight:
        return {DIAGONAL, -DIAGONAL};

    case Direction::ForwardLeft:
        return {-DIAGONAL, -DIAGONAL};

    case Direction::DownRight:
        return {DIAGONAL, DIAGONAL};

    case Direction::DownLeft:
        return {-DIAGONAL, DIAGONAL};
    }

    return {0.0, 1.0};
}

static qreal projectileRotation(Direction facing)
{
    switch (facing) {
    case Direction::Right:
        return 0.0;

    case Direction::ForwardRight:
        return -45.0;

    case Direction::Up:
        return -90.0;

    case Direction::ForwardLeft:
        return -135.0;

    case Direction::Left:
        return 180.0;

    case Direction::DownLeft:
        return 135.0;

    case Direction::Down:
        return 90.0;

    case Direction::DownRight:
        return 45.0;
    }

    return 0.0;
}

QRectF WorldCombatManager::swordBounds(const QRectF& playerBounds, Direction facing) const
{
    const qreal size = 48.0;

    switch (facing) {
    case Direction::Left:
        return QRectF(
            playerBounds.left() - size,
            playerBounds.center().y() - size / 2.0,
            size,
            size
        );

    case Direction::Right:
        return QRectF(
            playerBounds.right(),
            playerBounds.center().y() - size / 2.0,
            size,
            size
        );

    case Direction::Up:
    case Direction::ForwardLeft:
    case Direction::ForwardRight:
        return QRectF(
            playerBounds.center().x() - size / 2.0,
            playerBounds.top() - size,
            size,
            size
        );

    case Direction::Down:
    case Direction::DownLeft:
    case Direction::DownRight:
        return QRectF(
            playerBounds.center().x() - size / 2.0,
            playerBounds.bottom(),
            size,
            size
        );
    }

    return QRectF();
}

ActiveAttack* WorldCombatManager::createSwordSwing(const QRectF& playerBounds, Direction facing)
{
    if (!canPlayerSwordAttack())
        return nullptr;

    m_playerSwordCooldown = PLAYER_SWORD_COOLDOWN;

    ActiveAttack* attack = new ActiveAttack;
    attack->type = AttackType::Sword;
    attack->bounds = swordBounds(playerBounds, facing);
    attack->velocity = QPointF(0.0, 0.0);
    attack->damage = SWORD_DAMAGE;
    attack->lifetime = 0.12f;

    if (m_scene) {
        QGraphicsRectItem* item = m_scene->addRect(
            attack->bounds,
            QPen(QColor("#ffe066")),
            QBrush(QColor(255, 224, 102, 90))
        );

        item->setZValue(10);
        attack->visual = item;
    }

    m_activeAttacks.append(attack);
    return attack;
}

ActiveAttack* WorldCombatManager::shootArrow(const QRectF& playerBounds, Direction facing)
{
    if (!canPlayerShoot())
        return nullptr;

    m_playerShootCooldown = PLAYER_SHOOT_COOLDOWN;

    QPointF dir = directionVector(facing);
    QPointF start = playerBounds.center();

    static constexpr qreal ARROW_W = 65.0;
    static constexpr qreal ARROW_H = 25.0;

    ActiveAttack* attack = new ActiveAttack;
    attack->type = AttackType::Arrow;
    attack->bounds = QRectF(
        start.x() - ARROW_W / 2.0,
        start.y() - ARROW_H / 2.0,
        ARROW_W,
        ARROW_H
    );
    attack->velocity = dir * 360.0;
    attack->damage = ARROW_DAMAGE;
    attack->lifetime = 1.2f;

    if (m_scene) {

        QPixmap px("resources/sprites/arrow.png");

        QGraphicsPixmapItem* item = new QGraphicsPixmapItem(
            px.scaled(
                static_cast<int>(ARROW_W),
                static_cast<int>(ARROW_H),
                Qt::KeepAspectRatio,
                Qt::FastTransformation
            )
        );

        item->setTransformOriginPoint(item->boundingRect().center());
        item->setRotation(projectileRotation(facing));
        item->setPos(attack->bounds.topLeft());
        item->setZValue(5);

        m_scene->addItem(item);
        attack->visual = item;
    }

    m_activeAttacks.append(attack);
    return attack;
}

ActiveAttack* WorldCombatManager::shootFireball(const QRectF& playerBounds, Direction facing)
{
    if (!canPlayerShoot())
        return nullptr;

    m_playerShootCooldown = PLAYER_SHOOT_COOLDOWN;

    QPointF dir = directionVector(facing);
    QPointF start = playerBounds.center();

    static constexpr qreal FIREBALL_SIZE = 60.0;

    ActiveAttack* attack = new ActiveAttack;
    attack->type = AttackType::Fireball;
    attack->bounds = QRectF(
        start.x() - FIREBALL_SIZE / 2.0,
        start.y() - FIREBALL_SIZE / 2.0,
        FIREBALL_SIZE,
        FIREBALL_SIZE
    );
    attack->velocity = dir * 260.0;
    attack->damage = FIREBALL_DAMAGE;
    attack->lifetime = 1.5f;

    if (m_scene) {
        QPixmap px("resources/sprites/fireball.png");

        QGraphicsPixmapItem* item = new QGraphicsPixmapItem(
            px.scaled(
                static_cast<int>(FIREBALL_SIZE),
                static_cast<int>(FIREBALL_SIZE),
                Qt::KeepAspectRatio,
                Qt::FastTransformation
            )
        );

        item->setTransformOriginPoint(item->boundingRect().center());
        item->setRotation(projectileRotation(facing));
        item->setPos(attack->bounds.topLeft());
        item->setZValue(5);

        m_scene->addItem(item);
        attack->visual = item;
    }

    m_activeAttacks.append(attack);
    return attack;
}

const QList<ActiveAttack*>& WorldCombatManager::activeAttacks() const
{
    return m_activeAttacks;
}

void WorldCombatManager::expireAttack(ActiveAttack* attack)
{
    if (attack)
        attack->expired = true;
}

int WorldCombatManager::enemyAttackDamage(Enemy* enemy) const
{
    if (!enemy || !enemy->isAlive())
        return 0;

    return enemy->attackDamage();
}

void WorldCombatManager::damageEnemy(Enemy* enemy, int amount)
{
    if (!enemy || !enemy->isAlive() || amount <= 0)
        return;

    enemy->takeDamage(amount);
    emit enemyDamaged(enemy, amount);

    if (!enemy->isAlive())
        emit enemyDied(enemy);
}

void WorldCombatManager::damagePlayer(int amount)
{
    if (!m_player || !m_player->isAlive() || amount <= 0)
        return;

    m_player->takeDamage(amount);
    emit playerDamaged(amount);

    if (!m_player->isAlive())
        emit playerDied();
}

void WorldCombatManager::startEnemyAttackCooldown(Enemy* enemy)
{
    if (!enemy)
        return;

    m_enemyAttackCooldowns[enemy] = ENEMY_ATTACK_COOLDOWN;
}

bool WorldCombatManager::isPlayerAlive() const
{
    return m_player && m_player->isAlive();
}
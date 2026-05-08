#include "worldcombatmanager.h"

#include <algorithm>

#include <QBrush>
#include <QColor>
#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QPen>
#include <QPixmap>

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

    if (!m_player || m_player->getType() != CharacterType::Warrior)
        m_warriorSpecialActive = false;
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
    updateSpecial(deltaTime);
    updateAttacks(deltaTime);
    removeExpiredAttacks();
}

void WorldCombatManager::updateCooldowns(float deltaTime)
{
    m_playerSwordCooldown = std::max(0.0f, m_playerSwordCooldown - deltaTime);
    m_playerShootCooldown = std::max(0.0f, m_playerShootCooldown - deltaTime);

    for (auto it = m_enemyAttackCooldowns.begin(); it != m_enemyAttackCooldowns.end(); ++it)
        it.value() = std::max(0.0f, it.value() - deltaTime);
}

void WorldCombatManager::updateSpecial(float deltaTime)
{
    if (!m_warriorSpecialActive)
        return;

    if (!m_player || !m_player->isAlive() || m_player->getType() != CharacterType::Warrior) {
        m_warriorSpecialActive = false;
        return;
    }

    m_specialMeter = std::max(0.0f, m_specialMeter - WARRIOR_SPECIAL_DRAIN_PER_SECOND * deltaTime);

    if (m_specialMeter <= 0.0f)
        m_warriorSpecialActive = false;
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

void WorldCombatManager::addSpecialFromKill()
{
    m_specialMeter = std::min(1.0f, m_specialMeter + SPECIAL_GAIN_PER_KILL);
}

void WorldCombatManager::resetSpecialMeter()
{
    m_specialMeter = 0.0f;
    m_warriorSpecialActive = false;
}

int WorldCombatManager::applyPlayerDamageModifiers(int baseDamage) const
{
    if (!m_player)
        return baseDamage;

    if (m_player->getType() == CharacterType::Warrior && m_warriorSpecialActive)
        return baseDamage * 2;

    return baseDamage;
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

QList<ActiveAttack*> WorldCombatManager::createPlayerSpecial(const QRectF& playerBounds,
                                                             Direction facing)
{
    QList<ActiveAttack*> created;

    if (!m_player || !m_player->isAlive())
        return created;

    switch (m_player->getType()) {
    case CharacterType::Warrior:
        if (m_warriorSpecialActive) {
            m_warriorSpecialActive = false;
            return created;
        }

        if (m_specialMeter > 0.0f)
            m_warriorSpecialActive = true;

        return created;

    case CharacterType::Mage: {
        if (m_specialMeter + 0.0001f < MAGE_SPECIAL_COST)
            return created;

        m_specialMeter = std::max(0.0f, m_specialMeter - MAGE_SPECIAL_COST);

        const QList<Direction> dirs = {
            Direction::Right,
            Direction::ForwardRight,
            Direction::Up,
            Direction::ForwardLeft,
            Direction::Left,
            Direction::DownLeft,
            Direction::Down,
            Direction::DownRight
        };

        for (Direction dir : dirs) {
            ActiveAttack* attack = createFireballProjectile(
                playerBounds,
                dir,
                40.0,
                300.0,
                MAGE_SPECIAL_FIREBALL_DAMAGE,
                1.25f,
                false
            );

            if (attack)
                created.append(attack);
        }

        return created;
    }

    case CharacterType::Archer: {
        if (m_specialMeter + 0.0001f < ARCHER_SPECIAL_COST)
            return created;

        m_specialMeter = std::max(0.0f, m_specialMeter - ARCHER_SPECIAL_COST);

        ActiveAttack* attack = createArrowProjectile(
            playerBounds,
            facing,
            180.0,
            68.0,
            420.0,
            GIANT_ARROW_DAMAGE,
            1.45f,
            false,
            true
        );

        if (attack)
            created.append(attack);

        return created;
    }
    }

    return created;
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

ActiveAttack* WorldCombatManager::createSwordSwing(const QRectF& playerBounds,
                                                   Direction facing)
{
    if (!canPlayerSwordAttack())
        return nullptr;

    m_playerSwordCooldown = PLAYER_SWORD_COOLDOWN;

    ActiveAttack* attack = new ActiveAttack;
    attack->type = AttackType::Sword;
    attack->bounds = swordBounds(playerBounds, facing);
    attack->velocity = QPointF(0.0, 0.0);
    attack->damage = applyPlayerDamageModifiers(SWORD_DAMAGE);
    attack->lifetime = 0.12f;
    attack->piercing = false;

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

ActiveAttack* WorldCombatManager::shootArrow(const QRectF& playerBounds,
                                             Direction facing)
{
    return createArrowProjectile(
        playerBounds,
        facing,
        48.0,
        18.0,
        360.0,
        ARROW_DAMAGE,
        1.2f,
        true,
        false
    );
}

ActiveAttack* WorldCombatManager::shootFireball(const QRectF& playerBounds,
                                                Direction facing)
{
    return createFireballProjectile(
        playerBounds,
        facing,
        40.0,
        260.0,
        FIREBALL_DAMAGE,
        1.5f,
        true
    );
}

ActiveAttack* WorldCombatManager::createArrowProjectile(const QRectF& playerBounds,
                                                        Direction facing,
                                                        qreal width,
                                                        qreal height,
                                                        qreal speed,
                                                        int damage,
                                                        float lifetime,
                                                        bool startCooldown,
                                                        bool piercing)
{
    if (startCooldown) {
        if (!canPlayerShoot())
            return nullptr;

        m_playerShootCooldown = PLAYER_SHOOT_COOLDOWN;
    }

    QPointF dir = directionVector(facing);
    QPointF start = playerBounds.center();

    ActiveAttack* attack = new ActiveAttack;
    attack->type = AttackType::Arrow;
    attack->bounds = QRectF(
        start.x() - width / 2.0,
        start.y() - height / 2.0,
        width,
        height
    );
    attack->velocity = dir * speed;
    attack->damage = damage;
    attack->lifetime = lifetime;
    attack->piercing = piercing;

    if (m_scene) {
        QPixmap px(":/sprites/arrow.png");

        if (!px.isNull()) {
            QGraphicsPixmapItem* item = new QGraphicsPixmapItem(
                px.scaled(
                    static_cast<int>(width),
                    static_cast<int>(height),
                    Qt::KeepAspectRatio,
                    Qt::FastTransformation
                )
            );

            item->setTransformOriginPoint(item->boundingRect().center());
            item->setRotation(projectileRotation(facing));
            item->setPos(attack->bounds.topLeft());
            item->setZValue(10);

            m_scene->addItem(item);
            attack->visual = item;
        } else {
            QGraphicsRectItem* item = m_scene->addRect(
                attack->bounds,
                QPen(QColor("#c8a15a")),
                QBrush(QColor("#c8a15a"))
            );

            item->setZValue(10);
            attack->visual = item;
        }
    }

    m_activeAttacks.append(attack);
    return attack;
}

ActiveAttack* WorldCombatManager::createFireballProjectile(const QRectF& playerBounds,
                                                           Direction facing,
                                                           qreal size,
                                                           qreal speed,
                                                           int damage,
                                                           float lifetime,
                                                           bool startCooldown)
{
    if (startCooldown) {
        if (!canPlayerShoot())
            return nullptr;

        m_playerShootCooldown = PLAYER_SHOOT_COOLDOWN;
    }

    QPointF dir = directionVector(facing);
    QPointF start = playerBounds.center();

    ActiveAttack* attack = new ActiveAttack;
    attack->type = AttackType::Fireball;
    attack->bounds = QRectF(
        start.x() - size / 2.0,
        start.y() - size / 2.0,
        size,
        size
    );
    attack->velocity = dir * speed;
    attack->damage = damage;
    attack->lifetime = lifetime;
    attack->piercing = false;

    if (m_scene) {
        QPixmap px(":/sprites/fireball.png");

        if (!px.isNull()) {
            QGraphicsPixmapItem* item = new QGraphicsPixmapItem(
                px.scaled(
                    static_cast<int>(size),
                    static_cast<int>(size),
                    Qt::KeepAspectRatio,
                    Qt::FastTransformation
                )
            );

            item->setTransformOriginPoint(item->boundingRect().center());
            item->setRotation(projectileRotation(facing));
            item->setPos(attack->bounds.topLeft());
            item->setZValue(10);

            m_scene->addItem(item);
            attack->visual = item;
        } else {
            QGraphicsEllipseItem* item = m_scene->addEllipse(
                attack->bounds,
                QPen(QColor("#ff7043")),
                QBrush(QColor("#ff7043"))
            );

            item->setZValue(10);
            attack->visual = item;
        }
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
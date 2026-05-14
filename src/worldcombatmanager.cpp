#include "worldcombatmanager.h"

#include <algorithm>

#include <QBrush>
#include <QColor>
#include <QGraphicsRectItem>
#include <QPen>

#include "arrow.h"
#include "fireball.h"

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

        attack->update(deltaTime);
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

        const GameCombatManager::AttackConfig config = m_sharedCombat.fireballConfig(true);

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
            auto* attack = new Fireball(
                playerBounds,
                dir,
                config.width,
                config.speed,
                config.damage,
                config.lifetimeSeconds
            );

            attack->attachToScene(m_scene, dir);
            m_activeAttacks.append(attack);
            created.append(attack);
        }

        return created;
    }

    case CharacterType::Archer: {
        if (m_specialMeter + 0.0001f < ARCHER_SPECIAL_COST)
            return created;

        m_specialMeter = std::max(0.0f, m_specialMeter - ARCHER_SPECIAL_COST);

        const GameCombatManager::AttackConfig config = m_sharedCombat.arrowConfig(true);

        auto* attack = new Arrow(
            playerBounds,
            facing,
            config.width,
            config.height,
            config.speed,
            config.damage,
            config.lifetimeSeconds,
            config.piercing
        );

        attack->attachToScene(m_scene, facing);
        m_activeAttacks.append(attack);
        created.append(attack);

        return created;
    }
    }

    return created;
}

QRectF WorldCombatManager::swordBounds(const QRectF& playerBounds, Direction facing) const
{
    const GameCombatManager::AttackConfig config = m_sharedCombat.swordConfig(false);
    const qreal size = config.width;

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

    const GameCombatManager::AttackConfig config = m_sharedCombat.swordConfig(false);

    auto* attack = new Projectile(
        AttackType::Sword,
        swordBounds(playerBounds, facing),
        QPointF(0.0, 0.0),
        applyPlayerDamageModifiers(config.damage),
        config.lifetimeSeconds,
        false
    );

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
    if (!canPlayerShoot())
        return nullptr;

    m_playerShootCooldown = PLAYER_SHOOT_COOLDOWN;

    const GameCombatManager::AttackConfig config = m_sharedCombat.arrowConfig(false);

    auto* attack = new Arrow(
        playerBounds,
        facing,
        config.width,
        config.height,
        config.speed,
        config.damage,
        config.lifetimeSeconds,
        config.piercing
    );

    attack->attachToScene(m_scene, facing);
    m_activeAttacks.append(attack);
    return attack;
}

ActiveAttack* WorldCombatManager::shootFireball(const QRectF& playerBounds,
                                                Direction facing)
{
    if (!canPlayerShoot())
        return nullptr;

    m_playerShootCooldown = PLAYER_SHOOT_COOLDOWN;

    const GameCombatManager::AttackConfig config = m_sharedCombat.fireballConfig(false);

    auto* attack = new Fireball(
        playerBounds,
        facing,
        config.width,
        config.speed,
        config.damage,
        config.lifetimeSeconds
    );

    attack->attachToScene(m_scene, facing);
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
        attack->expire();
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

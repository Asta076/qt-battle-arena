#include "worldcombatmanager.h"

#include <algorithm>

WorldCombatManager::WorldCombatManager(QObject* parent)
    : QObject(parent)
{
}

void WorldCombatManager::setPlayer(Character* player)
{
    m_player = player;
}

void WorldCombatManager::registerEnemy(Enemy* enemy)
{
    if (!enemy) return;

    if (!m_enemyAttackCooldowns.contains(enemy)) {
        m_enemyAttackCooldowns.insert(enemy, 0.0f);
    }
}

void WorldCombatManager::unregisterEnemy(Enemy* enemy)
{
    if (!enemy) return;

    m_enemyAttackCooldowns.remove(enemy);
}

void WorldCombatManager::clearEnemies()
{
    m_enemyAttackCooldowns.clear();
}

void WorldCombatManager::update(float deltaTime)
{
    m_playerSwordCooldown = std::max(0.0f, m_playerSwordCooldown - deltaTime);
    m_playerShootCooldown = std::max(0.0f, m_playerShootCooldown - deltaTime);

    for (auto it = m_enemyAttackCooldowns.begin(); it != m_enemyAttackCooldowns.end(); ++it) {
        it.value() = std::max(0.0f, it.value() - deltaTime);
    }
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
    if (!enemy || !enemy->isAlive()) return false;
    return m_enemyAttackCooldowns.value(enemy, 0.0f) <= 0.0f;
}

int WorldCombatManager::playerSwordDamage()
{
    if (!m_player) return 0;

    return PLAYER_SWORD_BASE_DAMAGE;
}

int WorldCombatManager::playerProjectileDamage()
{
    if (!m_player) return 0;

    return PLAYER_PROJECTILE_BASE_DAMAGE;
}

int WorldCombatManager::enemyAttackDamage(Enemy* enemy) const
{
    if (!enemy || !enemy->isAlive()) return 0;

    return enemy->attackDamage();
}

void WorldCombatManager::damageEnemy(Enemy* enemy, int amount)
{
    if (!enemy || !enemy->isAlive() || amount <= 0) return;

    enemy->takeDamage(amount);
    emit enemyDamaged(enemy, amount);

    if (!enemy->isAlive()) {
        emit enemyDied(enemy);
    }
}

void WorldCombatManager::damagePlayer(int amount)
{
    if (!m_player || !m_player->isAlive() || amount <= 0) return;

    m_player->takeDamage(amount);
    emit playerDamaged(amount);

    if (!m_player->isAlive()) {
        emit playerDied();
    }
}

void WorldCombatManager::startPlayerSwordCooldown()
{
    m_playerSwordCooldown = PLAYER_SWORD_COOLDOWN;
}

void WorldCombatManager::startPlayerShootCooldown()
{
    m_playerShootCooldown = PLAYER_SHOOT_COOLDOWN;
}

void WorldCombatManager::startEnemyAttackCooldown(Enemy* enemy)
{
    if (!enemy) return;

    m_enemyAttackCooldowns[enemy] = ENEMY_ATTACK_COOLDOWN;
}

bool WorldCombatManager::isPlayerAlive() const
{
    return m_player && m_player->isAlive();
}

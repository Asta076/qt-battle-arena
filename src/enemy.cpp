#include "enemy.h"
#include <algorithm>

Enemy::Enemy(CharacterType type, const QString& name, int maxHp, int damage)
    : m_type(type),
      m_name(name),
      m_health(maxHp),
      m_maxHealth(maxHp),
      m_damage(damage)
{}

CharacterType Enemy::type() const { return m_type; }
QString Enemy::name() const { return m_name; }

int Enemy::health() const { return m_health; }
int Enemy::maxHealth() const { return m_maxHealth; }
int Enemy::damage() const { return m_damage; }

bool Enemy::isAlive() const {
    return m_health > 0;
}

void Enemy::takeDamage(int amount) {
    m_health = std::max(0, m_health - amount);
}

void Enemy::heal(int amount) {
    m_health = std::min(m_maxHealth, m_health + amount);
}

void Enemy::update(float) {}

int Enemy::attackDamage() const {
    return m_damage;
}

#include "character.h"
#include <QDebug>
#include <algorithm>

int Character::s_characterCount = 0;

Character::Character(const QString& name, int health, int attackPower)
    : m_name(name)
    , m_health(health)
    , m_maxHealth(health)
    , m_attackPower(attackPower)
{
    ++s_characterCount;
}

Character::~Character()
{
    --s_characterCount;
    qDebug() << m_name << "has been removed from the arena.";
}

QString Character::getName() const         { return m_name; }
int     Character::getMaxHealth() const    { return m_maxHealth; }
int     Character::getHealth() const       { return m_health; }
int     Character::getAttackPower() const  { return m_attackPower; }
int     Character::getCharacterCount()     { return s_characterCount; }

float Character::getHealthPercent() const
{
    if (m_maxHealth == 0) return 0.0f;
    return static_cast<float>(m_health) / static_cast<float>(m_maxHealth);
}

bool Character::isAlive() const
{
    return m_health > 0;
}

void Character::takeDamage(int damage)
{
    m_health = std::max(0, m_health - damage);
    qDebug() << m_name << "took" << damage
             << "damage. Health remaining:" << m_health;
}
void Character::heal(int amount)
{
    m_health = std::min(m_maxHealth, m_health + amount);
}

void Character::resetHealth()
{
    m_health = m_maxHealth;
}

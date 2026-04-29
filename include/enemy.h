#pragma once

#include <QString>
#include <QRectF>
#include "character.h"

class Enemy {
public:
    Enemy(CharacterType type, const QString& name, int maxHp, int damage);
    virtual ~Enemy() = default;

    CharacterType type() const;
    QString name() const;

    int health() const;
    int maxHealth() const;
    int damage() const;
    bool isAlive() const;

    void takeDamage(int amount);
    void heal(int amount);

    virtual void update(float deltaTime);
    virtual int attackDamage() const;

private:
    CharacterType m_type;
    QString m_name;

    int m_health;
    int m_maxHealth;
    int m_damage;
};

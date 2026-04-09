#pragma once
#include <QString>

enum class CharacterType { Warrior, Mage, Archer };

class Character {
public:
    Character(const QString& name, int health, int attackPower);
    virtual ~Character();

    // Getters
    QString       getName()         const;
    int           getMaxHealth()    const;
    float         getHealthPercent()const;   // 0.0 – 1.0, used by HealthBarWidget
    bool          isAlive()         const;
    virtual CharacterType getType() const = 0;

    // Combat interface
    virtual int   attack()         const = 0;
    virtual int   specialAbility() const = 0;
    void          takeDamage(int damage);

    // Static
    static int    getCharacterCount();
    void heal(int amount);
    void resetHealth();


protected:
    int getAttackPower() const;
    int getHealth()      const;

private:
    QString m_name;
    int     m_health;
    int     m_maxHealth;
    int     m_attackPower;

    static int s_characterCount;
};

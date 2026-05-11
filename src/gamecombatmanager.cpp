#include "gamecombatmanager.h"

#include <algorithm>

int GameCombatManager::maxHealthFor(CharacterType type) const
{
    switch (type) {
    case CharacterType::Warrior: return 120;
    case CharacterType::Archer:  return 90;
    case CharacterType::Mage:    return 100;
    }
    return 100;
}

int GameCombatManager::meleeDamageFor(CharacterType type) const
{
    switch (type) {
    case CharacterType::Warrior: return 25;
    case CharacterType::Archer:  return 18;
    case CharacterType::Mage:    return 20;
    }
    return 20;
}

int GameCombatManager::projectileDamageFor(CharacterType type) const
{
    switch (type) {
    case CharacterType::Archer:  return 18;
    case CharacterType::Mage:    return 30;
    case CharacterType::Warrior: return meleeDamageFor(type);
    }
    return 18;
}

int GameCombatManager::finalDamageAfterBlock(int rawDamage, bool blocking) const
{
    if (!blocking)
        return rawDamage;

    return std::max(1, rawDamage * BLOCK_DAMAGE_PERCENT / 100);
}

GameCombatManager::AttackConfig GameCombatManager::swordConfig(bool powered) const
{
    AttackConfig config;
    config.damage = meleeDamageFor(CharacterType::Warrior) * (powered ? 2 : 1);
    config.width = 48.0;
    config.height = 48.0;
    config.lifetimeSeconds = 0.12f;
    return config;
}

GameCombatManager::AttackConfig GameCombatManager::arrowConfig(bool giant) const
{
    AttackConfig config;
    config.damage = giant ? 45 : projectileDamageFor(CharacterType::Archer);
    config.speed = giant ? 420.0 : 360.0;
    config.width = giant ? 180.0 : 48.0;
    config.height = giant ? 68.0 : 18.0;
    config.lifetimeSeconds = giant ? 1.45f : 1.2f;
    config.piercing = giant;
    return config;
}

GameCombatManager::AttackConfig GameCombatManager::fireballConfig(bool special) const
{
    AttackConfig config;
    config.damage = special ? 24 : projectileDamageFor(CharacterType::Mage);
    config.speed = special ? 300.0 : 260.0;
    config.width = special ? 40.0 : 40.0;
    config.height = special ? 40.0 : 40.0;
    config.lifetimeSeconds = special ? 1.25f : 1.5f;
    return config;
}

qreal GameCombatManager::projectileSpeedFor(CharacterType type) const
{
    if (type == CharacterType::Mage)
        return fireballConfig(false).speed;
    if (type == CharacterType::Archer)
        return arrowConfig(false).speed;
    return 0.0;
}

int GameCombatManager::projectileMaxTicksFor(CharacterType) const
{
    return 90;
}

QPointF GameCombatManager::directionVector(Direction dir) const
{
    static constexpr qreal D = 0.70710678;

    switch (dir) {
    case Direction::Right:        return {1.0, 0.0};
    case Direction::ForwardRight: return {D, -D};
    case Direction::Up:           return {0.0, -1.0};
    case Direction::ForwardLeft:  return {-D, -D};
    case Direction::Left:         return {-1.0, 0.0};
    case Direction::DownLeft:     return {-D, D};
    case Direction::Down:         return {0.0, 1.0};
    case Direction::DownRight:    return {D, D};
    }

    return {0.0, 1.0};
}

QRectF GameCombatManager::meleeHitBox(const QPointF& actorPos,
                                      qreal actorW,
                                      qreal actorH,
                                      Direction facing) const
{
    QPointF center(actorPos.x() + actorW / 2.0, actorPos.y() + actorH / 2.0);
    QPointF hitCenter = center + directionVector(facing) * 48.0;

    return QRectF(hitCenter.x() - 28.0,
                  hitCenter.y() - 28.0,
                  56.0,
                  56.0);
}

CharacterType GameCombatManager::projectileTypeFor(CharacterType attacker) const
{
    return attacker;
}

bool GameCombatManager::usesProjectile(CharacterType attacker) const
{
    return attacker == CharacterType::Archer || attacker == CharacterType::Mage;
}

#include "pvplogicmanager.h"

QPointF PvpLogicManager::playerVelocity(const QSet<int>& heldKeys,
                                        int playerNumber,
                                        qreal baseSpeed,
                                        bool blocking) const
{
    const auto scheme = playerNumber == 1
        ? GameplayMovementManager::ControlScheme::Wasd
        : GameplayMovementManager::ControlScheme::Arrows;

    return m_movement.velocityFromKeys(
        heldKeys,
        baseSpeed,
        scheme,
        blocking ? BLOCK_SPEED_MULTIPLIER : 1.0
    );
}

Direction PvpLogicManager::directionFromVelocity(const QPointF& velocity,
                                                 Direction fallback) const
{
    return m_movement.directionFromVelocity(velocity, fallback);
}

QPointF PvpLogicManager::clampToArena(const QPointF& pos,
                                      qreal actorW,
                                      qreal actorH,
                                      const QRectF& arenaBounds) const
{
    return m_movement.clampToBounds(pos, actorW, actorH, arenaBounds);
}

int PvpLogicManager::maxHpFor(CharacterType type) const
{
    return m_combat.maxHealthFor(type);
}

int PvpLogicManager::projectileDamageFor(CharacterType type) const
{
    return m_combat.projectileDamageFor(type);
}

int PvpLogicManager::meleeDamageFor(CharacterType type) const
{
    return m_combat.meleeDamageFor(type);
}

int PvpLogicManager::damageAfterBlock(int rawDamage, bool blocking) const
{
    return m_combat.finalDamageAfterBlock(rawDamage, blocking);
}

qreal PvpLogicManager::projectileSpeed() const
{
    return PROJECTILE_SPEED;
}

int PvpLogicManager::projectileMaxTicks() const
{
    return PROJECTILE_MAX_TICKS;
}

qreal PvpLogicManager::knockbackDistance() const
{
    return KNOCKBACK_DISTANCE;
}

int PvpLogicManager::hitFlashTicks() const
{
    return HIT_FLASH_TICKS;
}

QPointF PvpLogicManager::directionVector(Direction dir) const
{
    return m_combat.directionVector(dir);
}

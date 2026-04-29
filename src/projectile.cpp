#include "projectile.h"

Projectile::Projectile(QPointF position, QPointF velocity, int damage)
    : m_position(position),
      m_velocity(velocity),
      m_damage(damage)
{}

void Projectile::update(float deltaTime) {
    m_position += m_velocity * deltaTime;
}

QPointF Projectile::position() const {
    return m_position;
}

QRectF Projectile::bounds() const {
    return QRectF(m_position.x(), m_position.y(), 16, 16);
}

int Projectile::damage() const {
    return m_damage;
}

bool Projectile::expired() const {
    return m_expired;
}

void Projectile::expire() {
    m_expired = true;
}

#pragma once

#include <QPointF>
#include <QRectF>

class Projectile {
public:
    Projectile(QPointF position, QPointF velocity, int damage);
    virtual ~Projectile() = default;

    virtual void update(float deltaTime);

    QPointF position() const;
    QRectF bounds() const;

    int damage() const;
    bool expired() const;
    void expire();

protected:
    QPointF m_position;
    QPointF m_velocity;

    int m_damage;
    bool m_expired = false;
};

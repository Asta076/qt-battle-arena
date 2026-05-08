#pragma once

#include <QList>
#include <QPixmap>
#include <QPointF>
#include <QSet>
#include <QTimer>
#include <QWidget>

#include "character.h"

class QPaintEvent;
class QKeyEvent;
class QPainter;

enum class PvpDirection {
    Down = 0,
    DownRight = 1,
    Right = 2,
    UpRight = 3,
    Up = 4,
    UpLeft = 5,
    Left = 6,
    DownLeft = 7
};

struct PvpFighterAnim {
    CharacterType type = CharacterType::Warrior;

    QPixmap movementSheet;
    QPixmap attackSheet;

    QPointF pos;

    PvpDirection facing = PvpDirection::Down;

    bool isMoving = false;

    int frameIndex = 0;
    int tickAccum = 0;

    bool isAttacking = false;
    int attackFrameIndex = 0;
    int attackTickAccum = 0;
};

struct PvpProjectile {
    QPointF pos;
    QPointF velocity;
    int owner = 0;
    int lifeTicks = 0;
};

class PvpArenaWidget : public QWidget {
    Q_OBJECT

public:
    explicit PvpArenaWidget(QWidget* parent = nullptr);

    void activate();
    void deactivate();

    void setFighters(CharacterType p1Type, CharacterType p2Type);

signals:
    void backToMenu();

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private slots:
    void onTick();

private:
    void resetPlayers();

    QPointF velocityForPlayer1() const;
    QPointF velocityForPlayer2() const;
    QPointF clampToArena(const QPointF& pos) const;

    PvpDirection directionFromVelocity(const QPointF& velocity,
                                       PvpDirection fallback) const;

    void updateFighterAnimation(PvpFighterAnim& fighter, bool isMoving);
    void startAttack(PvpFighterAnim& fighter, int owner);

    QPointF directionVector(PvpDirection dir) const;
    void spawnProjectile(const PvpFighterAnim& fighter, int owner);
    void updateProjectiles();
    void drawProjectiles(QPainter& painter);

    void drawPlayer(QPainter& painter,
                    const PvpFighterAnim& fighter,
                    const QString& label,
                    const QColor& outlineColor);

    QPixmap cropFrame(const PvpFighterAnim& fighter) const;

    QString movementSheetFor(CharacterType type) const;
    QString attackSheetFor(CharacterType type) const;

    static constexpr qreal WORLD_W = 960.0;
    static constexpr qreal WORLD_H = 720.0;

    static constexpr qreal PLAYER_W = 72.0;
    static constexpr qreal PLAYER_H = 88.0;

    static constexpr int FRAME_W = 68;
    static constexpr int FRAME_H = 68;

    static constexpr int WALK_FRAMES = 6;
    static constexpr int TICKS_PER_FRAME = 5;

    static constexpr int ATTACK_FRAMES = 7;
    static constexpr int ATTACK_TICKS_PER_FRAME = 4;

    static constexpr qreal SPEED = 4.0;

    static constexpr qreal PROJECTILE_SPEED = 9.0;
    static constexpr int PROJECTILE_MAX_TICKS = 90;

    QPixmap m_arenaBackground;

    PvpFighterAnim m_p1;
    PvpFighterAnim m_p2;

    QList<PvpProjectile> m_projectiles;

    QTimer m_ticker;
    QSet<int> m_heldKeys;
};
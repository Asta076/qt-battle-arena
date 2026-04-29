#pragma once

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QTimer>
#include <QKeyEvent>
#include <QSet>
#include <QList>
#include <QPixmap>

#include "character.h"
#include "goldhudwidget.h"
#include "playercontroller.h"
#include "overworldwidget.h"   // Direction, walkRow(), idleCol()

class AudioManager;
class PauseOverlayWidget;

struct DungeonSpriteSheet
{
    QPixmap pixmap;

    static constexpr int FRAME_W = 68;
    static constexpr int FRAME_H = 68;

    QPixmap frame(int col, int row) const
    {
        return pixmap.copy(col * FRAME_W, row * FRAME_H, FRAME_W, FRAME_H);
    }
};

class DungeonPlayerSprite : public QGraphicsPixmapItem
{
public:
    explicit DungeonPlayerSprite(const DungeonSpriteSheet& sheet,
                                 QGraphicsItem* parent = nullptr);

    static constexpr qreal W = 96.0;
    static constexpr qreal H = 96.0;

    void updateAnimation(bool isMoving, Direction facingDir);

private:
    void setWalkAnim(Direction dir);
    void setIdleFrame(Direction dir);
    void applyFrame();

    const DungeonSpriteSheet& m_sheet;
    bool m_isIdle = true;
    Direction m_facing = Direction::Down;
    int m_frameIndex = 0;
    int m_tickAccum = 0;

    static constexpr int TICKS_PER_FRAME = 5;
    static constexpr int WALK_FRAMES = 6;

    QGraphicsEllipseItem* m_shadow = nullptr;
};

class EnemySprite : public QGraphicsPixmapItem
{
public:
    explicit EnemySprite(CharacterType type, const QString& name,
                         QGraphicsItem* parent = nullptr);

    static constexpr qreal W = 28;
    static constexpr qreal H = 28;

    CharacterType enemyType() const { return m_type; }
    QString enemyName() const { return m_name; }

    void patrol(const QRectF& worldBounds);
    void updateAnimation();

private:
    CharacterType m_type;
    QString m_name;

    QPixmap m_sprite;
    QGraphicsEllipseItem* m_shadow = nullptr;
    QGraphicsTextItem* m_nameLabel = nullptr;

    qreal m_vx = 0;
    qreal m_vy = 0;

    int m_frameIndex = 0;
    int m_tickAccum = 0;
};

class DungeonWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DungeonWidget(AudioManager* audio, QWidget* parent = nullptr);
    ~DungeonWidget() override = default;

    void activate();
    void deactivate();
    void setGold(int gold);

signals:
    void exitedDungeon();
    void backToMenu();

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;

private slots:
    void onTick();

private:
    PlayerController m_controller;

    QGraphicsScene* m_scene = nullptr;
    QGraphicsView* m_view = nullptr;
    GoldHudWidget* m_goldHud = nullptr;

    DungeonPlayerSprite* m_player = nullptr;
    QGraphicsRectItem* m_exitZone = nullptr;

    QList<EnemySprite*> m_enemies;

    QTimer m_ticker;
    QSet<int> m_heldKeys;

    static constexpr int WORLD_W = 800;
    static constexpr int WORLD_H = 600;

    static constexpr qreal PW = DungeonPlayerSprite::W;
    static constexpr qreal PH = DungeonPlayerSprite::H;
    static constexpr int SPEED = 3;

    DungeonSpriteSheet m_sheet;

    void buildScene();
    void placePlayer();
    void spawnEnemies();
    void movePlayer();
    void patrolEnemies();
    void checkCollisions();
    void fitView();
    void togglePause();

    AudioManager* m_audio = nullptr;
    PauseOverlayWidget* m_pauseOverlay = nullptr;
    bool m_paused = false;
};

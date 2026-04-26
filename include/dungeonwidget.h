#pragma once

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QTimer>
#include <QKeyEvent>
#include <QSet>
#include <QList>
#include <QPixmap>
#include "character.h"
#include "goldhudwidget.h"
#include "playercontroller.h"
#include "direction.h"

class AudioManager;

// ─────────────────────────────────────────────────────────────────────────────
//  SpriteSheet for dungeon (same pattern as overworld)
// ─────────────────────────────────────────────────────────────────────────────
struct DungeonSpriteSheet
{
    QPixmap pixmap;
    static constexpr int FRAME_W = 68;
    static constexpr int FRAME_H = 68;

    QPixmap frame(int col, int row) const {
        return pixmap.copy(col * FRAME_W, row * FRAME_H, FRAME_W, FRAME_H);
    }
};



// ─────────────────────────────────────────────────────────────────────────────
//  DungeonPlayerSprite — animated player sprite for dungeon exploration
// ─────────────────────────────────────────────────────────────────────────────
class DungeonPlayerSprite : public QGraphicsPixmapItem
{
public:
    static constexpr qreal W = 64.0;
    static constexpr qreal H = 64.0;

    explicit DungeonPlayerSprite(const DungeonSpriteSheet &sheet, 
                                 QGraphicsItem *parent = nullptr);

    void updateAnimation(bool isMoving, Direction facingDir);

private:
    void setWalkAnim(Direction dir);
    void setIdleFrame(Direction dir);
    void applyFrame();

    const DungeonSpriteSheet &m_sheet;
    bool      m_isIdle     = true;
    Direction m_facing     = Direction::Down;
    int       m_frameIndex = 0;
    int       m_tickAccum  = 0;

    static constexpr int TICKS_PER_FRAME = 5;
    static constexpr int WALK_FRAMES     = 6;

    QGraphicsEllipseItem *m_shadow = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────
//  EnemySprite — roaming enemies with proper sprites
// ─────────────────────────────────────────────────────────────────────────────
class EnemySprite : public QGraphicsPixmapItem
{
public:
    static constexpr qreal W = 56;
    static constexpr qreal H = 56;

    explicit EnemySprite(CharacterType type, const QString &name,
                         QGraphicsItem *parent = nullptr);

    CharacterType enemyType() const { return m_type; }
    QString       enemyName() const { return m_name; }

    void patrol(const QRectF &worldBounds);
    void updateAnimation();  // cycles through idle frames

private:
    CharacterType m_type;
    QString       m_name;
    QPixmap       m_sprite;
    
    qreal m_vx;
    qreal m_vy;
    
    int   m_frameIndex = 0;
    int   m_tickAccum  = 0;
    
    QGraphicsEllipseItem *m_shadow = nullptr;
    QGraphicsTextItem    *m_nameLabel = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────
//  DungeonWidget — modernized with sprite support
// ─────────────────────────────────────────────────────────────────────────────
class DungeonWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DungeonWidget(AudioManager *audio, QWidget *parent = nullptr);
    ~DungeonWidget() override = default;

    void activate();
    void deactivate();
    void setGold(int gold);

signals:
    void battleTriggered(CharacterType enemyType, const QString &enemyName);
    void exitedDungeon();
    void backToMenu();
    void saveRequested();

protected:
    void keyPressEvent  (QKeyEvent   *e) override;
    void keyReleaseEvent(QKeyEvent   *e) override;
    void resizeEvent    (QResizeEvent *e) override;

private slots:
    void onTick();

private:
    PlayerController m_controller;
    
    // Scene
    QGraphicsScene       *m_scene     = nullptr;
    QGraphicsView        *m_view      = nullptr;
    GoldHudWidget        *m_goldHud   = nullptr;
    
    // Player sprite (using same sheet as overworld)
    DungeonSpriteSheet    m_sheet;
    DungeonPlayerSprite  *m_player    = nullptr;
    
    // Exit portal
    QGraphicsRectItem    *m_exitZone  = nullptr;
    
    // Enemies with sprites
    QList<EnemySprite *>  m_enemies;
    
    // Game loop
    QTimer    m_ticker;
    QSet<int> m_heldKeys;
    
    // World dimensions
    static constexpr int WORLD_W = 800;
    static constexpr int WORLD_H = 600;

    // Helpers
    void buildScene();
    void placePlayer();
    void spawnEnemies();
    void checkTriggers();
    void fitView();
    void togglePause();

    AudioManager       *m_audio        = nullptr;
    PauseOverlayWidget *m_pauseOverlay = nullptr;
    bool                m_paused       = false;
};

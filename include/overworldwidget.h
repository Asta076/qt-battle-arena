#pragma once

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QTimer>
#include <QSet>
#include <QPixmap>
#include <QKeyEvent>

// ═════════════════════════════════════════════════════════════════════════════
//  SpriteSheet
//
//  Physical layout:  8 columns × 9 rows, each cell 68 × 68 px  (544 × 612 total)
//
//  Row 0        — Idle stances:  one static frame per column (8 directions)
//  Rows 1 – 8  — Walk cycles:   6 animated frames (cols 0-5); cols 6-7 empty
// ═════════════════════════════════════════════════════════════════════════════
struct SpriteSheet
{
    QPixmap pixmap;

    static constexpr int FRAME_W = 68;
    static constexpr int FRAME_H = 68;

    QPixmap frame(int col, int row) const
    {
        return pixmap.copy(col * FRAME_W, row * FRAME_H, FRAME_W, FRAME_H);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  Direction enum
//
//  Each enumerator value N encodes:
//    idle frame  → row 0, column N
//    walk cycle  → row N+1, columns 0-5
//
//  Directions confirmed by visual inspection of individual frames:
//    Col/Row 0 → SE   (right, facing camera-right)
//    Col/Row 1 → N    (up,   back to camera)
//    Col/Row 2 → SW   (left-back diagonal)
//    Col/Row 3 → W    (left)
//    Col/Row 4 → NE   (up-right diagonal)
//    Col/Row 5 → SW2  (down-left, slight crouch)
//    Col/Row 6 → E    (right-forward)
//    Col/Row 7 → S    (down / toward camera)
// ─────────────────────────────────────────────────────────────────────────────
enum class Direction : int
{
    SE   = 0,
    N    = 1,
    SW   = 2,
    W    = 3,
    NE   = 4,
    SW2  = 5,
    E    = 6,
    S    = 7
};

// Helper: walk row for a direction
inline int walkRow(Direction d) { return static_cast<int>(d) + 1; }
// Helper: idle column for a direction
inline int idleCol(Direction d) { return static_cast<int>(d); }

// ─────────────────────────────────────────────────────────────────────────────
//  WASD → Direction mapping
//
//  Pure cardinal keys map to the four true cardinal directions.
//  Diagonal combinations map to the nearest diagonal direction.
//
//   W        → N    (row 2, back to camera)
//   S        → S    (row 8, toward camera)
//   A        → W    (row 4, left)
//   D        → E    (row 7, right-forward)
//   W+D      → NE   (row 5)
//   W+A      → SW   (row 3)  [best available NW approximation]
//   S+D      → SE   (row 1)
//   S+A      → SW2  (row 6)
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
//  PlayerSprite
// ─────────────────────────────────────────────────────────────────────────────
class PlayerSprite : public QGraphicsPixmapItem
{
public:
    static constexpr qreal W     = 96.0;   // logical scene size (scaled from 68px)
    static constexpr qreal H     = 96.0;
    static constexpr qreal SPEED = 3.0;

    explicit PlayerSprite(const SpriteSheet &sheet,
                          QGraphicsItem     *parent = nullptr);

    // Call once per physics tick from OverworldWidget::onTick()
    void step(const QSet<int> &heldKeys, const QRectF &worldBounds);

private:
    void setWalkAnim(Direction dir);
    void setIdleFrame(Direction dir);
    void applyFrame();

    const SpriteSheet &m_sheet;

    bool      m_isIdle     = true;
    Direction m_facing     = Direction::S;   // start facing toward camera
    int       m_frameIndex = 0;
    int       m_tickAccum  = 0;

    static constexpr int TICKS_PER_FRAME = 5;   // animation speed (~12 fps @ 60)
    static constexpr int WALK_FRAMES     = 6;   // frames per walk row (cols 0-5)
};

// ═════════════════════════════════════════════════════════════════════════════
//  OverworldWidget
// ═════════════════════════════════════════════════════════════════════════════
class OverworldWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OverworldWidget(QWidget *parent = nullptr);

    void activate();
    void deactivate();

signals:
    void dungeonEntered();
    void backToMenu();

protected:
    void keyPressEvent(QKeyEvent   *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    void resizeEvent(QResizeEvent  *e) override;

private slots:
    void onTick();

private:
    void buildScene();
    void placePlayer();
    void checkTriggers();
    void fitView();

    static constexpr qreal WORLD_W = 800;
    static constexpr qreal WORLD_H = 600;

    QGraphicsScene    *m_scene       = nullptr;
    QGraphicsView     *m_view        = nullptr;
    PlayerSprite      *m_player      = nullptr;
    QGraphicsRectItem *m_dungeonZone = nullptr;

    QTimer    m_ticker;
    QSet<int> m_heldKeys;

    SpriteSheet m_sheet;
};

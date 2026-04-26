#pragma once

// Qt stuff we need
#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QTimer>
#include <QSet>
#include <QPixmap>
#include <QKeyEvent>

#include "goldhudwidget.h"
#include "playercontroller.h"

// forward declare so we dont have to include the whole header here
class AudioManager;


// ============================================================
//  SpriteSheet
//  Holds the player sprite sheet and lets us grab individual frames
//
//  The sheet is 8 columns x 9 rows, each frame is 68x68 pixels
//  Row 0 = idle poses, rows 1-8 = walking animations
// ============================================================
struct SpriteSheet
{
    QPixmap pixmap;

    // size of each frame on the sheet
    static constexpr int FRAME_W = 68;
    static constexpr int FRAME_H = 68;

    // returns a single frame cropped from the sheet
    QPixmap frame(int col, int row) const
    {
        return pixmap.copy(col * FRAME_W, row * FRAME_H, FRAME_W, FRAME_H);
    }
};


// ============================================================
//  Direction
//  The 8 directions the player can face/walk
//
//  The int value maps directly to which row/column to use on the sheet:
//    idle frame  -> row 0, column = Direction value
//    walk cycle  -> row = Direction value + 1, columns 0-5
// ============================================================
enum class Direction : int
{
    Right        = 0,
    Up           = 1,
    ForwardRight = 2,
    ForwardLeft  = 3,
    Down         = 4,
    DownRight    = 5,
    DownLeft     = 6,
    Left         = 7
};

// helpers to get row/col from a direction (cleaner than casting everywhere)
inline int walkRow(Direction d) { return static_cast<int>(d) + 1; }
inline int idleCol(Direction d) { return static_cast<int>(d); }


// ============================================================
//  PlayerSprite
//  The player character on the overworld map
//  Handles movement, animation, and the shadow underneath
// ============================================================
class PlayerSprite : public QGraphicsPixmapItem
{
public:
    // logical display size and movement speed (in scene units)
    static constexpr qreal W     = 96.0;
    static constexpr qreal H     = 96.0;
    static constexpr qreal SPEED = 3.0;

    explicit PlayerSprite(const SpriteSheet &sheet, QGraphicsItem *parent = nullptr);

    // called once per game tick to move the player and update animation
    void step(const QSet<int> &heldKeys, const QRectF &worldBounds,
              const QRectF &solidCollider = QRectF());

private:
    // animation helpers
    void setWalkAnim(Direction dir);
    void setIdleFrame(Direction dir);
    void applyFrame();

    const SpriteSheet &m_sheet;

    // animation state
    bool      m_isIdle     = true;
    Direction m_facing     = Direction::Down;
    int       m_frameIndex = 0;
    int       m_tickAccum  = 0;

    // how many ticks before we advance to the next animation frame (~12fps at 60hz)
    static constexpr int TICKS_PER_FRAME = 5;
    static constexpr int WALK_FRAMES     = 6;

    // the shadow ellipse drawn under the player
    QGraphicsEllipseItem *m_shadow = nullptr;
};


// ============================================================
//  OverworldWidget
//  The main overworld/exploration screen
//  Player can walk around, enter the dungeon, visit the house, or open the menu
// ============================================================
class OverworldWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OverworldWidget(AudioManager *audio, QWidget *parent = nullptr);

    // call these when switching to/from this screen
    void activate();
    void deactivate();

    // updates the gold counter in the HUD
    void setGold(int gold);

signals:
    void dungeonEntered();
    void houseEntered();
    void shopEntered();
    void backToMenu();
    void saveRequested();

protected:
    // Qt event overrides
    void keyPressEvent(QKeyEvent   *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    void resizeEvent(QResizeEvent  *e) override;
    void showEvent(QShowEvent      *e) override;

private slots:
    void onTick(); // runs every 16ms (roughly 60fps)

private:
    
    PlayerController m_controller;
    // --- setup ---
    void buildScene();       // creates all the tiles, trees, buildings etc.
    void buildPauseOverlay(); // creates the pause menu widget
    void placePlayer();      // puts player in the center of the map

    // --- game loop ---
    void checkTriggers();    // checks if player walked into a zone

    // --- misc ---
    void fitView();          // scales the view to fill the widget
    void togglePause();      // shows/hides the pause overlay

    // world size in scene units
    static constexpr qreal WORLD_W = 800;
    static constexpr qreal WORLD_H = 600;

    // --- scene objects ---
    QGraphicsScene    *m_scene             = nullptr;
    QGraphicsView     *m_view              = nullptr;
    PlayerSprite      *m_player            = nullptr;
    QGraphicsRectItem *m_dungeonZone       = nullptr;
    QGraphicsRectItem *m_houseCollider     = nullptr;
    QGraphicsRectItem *m_shopZone          = nullptr;
    QGraphicsRectItem *m_houseEntranceZone = nullptr;
    GoldHudWidget     *m_goldHud           = nullptr;

    // --- game loop stuff ---
    QTimer    m_ticker;
    QSet<int> m_heldKeys;

    // --- assets ---
    SpriteSheet m_sheet;

    // --- other ---
    AudioManager *m_audio        = nullptr;
    QWidget      *m_pauseOverlay = nullptr;
    bool          m_paused       = false;
};

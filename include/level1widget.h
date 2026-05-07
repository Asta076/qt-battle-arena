#pragma once

// ─────────────────────────────────────────────────────────────────────────────
//  Level1Widget  —  "Forest Path"
//
//  A self-contained game level that reuses the existing dungeon sprite classes
//  (DungeonPlayerSprite, EnemySprite, DungeonSpriteSheet) and infrastructure
//  (PlayerController, SpriteCache, AudioManager, PauseOverlayWidget, GoldHudWidget).
//
//  GUI layout comes from level1widget.ui (QGraphicsView as the canvas).
//  Any level-specific gameplay data lives in level1data.h (separate file).
//
//  Integration checklist for MainWindow:
//    1. Add  Level1Widget* m_level1 = nullptr;  to mainwindow.h
//    2. In buildUI(): m_level1 = new Level1Widget(m_audio, this); m_stack->addWidget(m_level1);
//    3. Connect m_level1->battleTriggered  → onBattleTriggered
//               m_level1->exitedLevel      → onExitedLevel1   (go back to overworld)
//               m_level1->backToMenu       → onBackToMenu
//    4. Add Level1 to the trigger in OverworldWidget (or DungeonWidget) that emits the signal.
//    5. In CMakeLists add:  include/level1widget.h  src/level1widget.cpp  src/level1widget.ui
// ─────────────────────────────────────────────────────────────────────────────

#include <QWidget>
#include <QTimer>
#include <QSet>
#include <QList>

// ── Repo headers — no new logic added here ────────────────────────────────────
#include "character.h"
#include "dungeonwidget.h"      // reuses DungeonPlayerSprite, EnemySprite, DungeonSpriteSheet
#include "goldhudwidget.h"
#include "playercontroller.h"
#include "leveldef.h"
#include "playerprofile.h"

class AudioManager;
class PauseOverlayWidget;
class QGraphicsScene;
class QGraphicsView;
class QGraphicsRectItem;
class QKeyEvent;
class QResizeEvent;

QT_BEGIN_NAMESPACE
namespace Ui { class Level1Widget; }
QT_END_NAMESPACE

// ─────────────────────────────────────────────────────────────────────────────

class Level1Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Level1Widget(AudioManager* audio, QWidget* parent = nullptr);
    ~Level1Widget() override;

    // Call from MainWindow when switching to / away from this screen
    void activate(const LevelDef& level, const PlayerProfile& profile);
    void deactivate();

    // Keeps the gold HUD in sync with the player profile
    void setGold(int gold);

signals:
    void battleTriggered(CharacterType enemyType, const QString& enemyName);
    void exitedLevel();       // player reached the exit portal
    void backToMenu();        // player chose Main Menu from the pause overlay
    void bossTriggered(const LevelDef& level);

protected:
    void keyPressEvent  (QKeyEvent*   e) override;
    void keyReleaseEvent(QKeyEvent*   e) override;
    void resizeEvent    (QResizeEvent* e) override;

private slots:
    void onTick();            // fired every ~16 ms by m_ticker

private:
    // ── Qt Designer form ──────────────────────────────────────────────────────
    Ui::Level1Widget* ui;

    // ── Scene objects ─────────────────────────────────────────────────────────
    QGraphicsScene*      m_scene    = nullptr;
    QGraphicsView*       m_view     = nullptr;     // promoted widget from .ui
    DungeonPlayerSprite* m_player   = nullptr;
    QGraphicsRectItem*   m_exitZone = nullptr;
    QList<EnemySprite*>  m_enemies;

    // ── HUD / overlays ────────────────────────────────────────────────────────
    GoldHudWidget*      m_goldHud      = nullptr;
    PauseOverlayWidget* m_pauseOverlay = nullptr;

    // ── Game-loop state ───────────────────────────────────────────────────────
    QTimer           m_ticker;
    QSet<int>        m_heldKeys;
    PlayerController m_controller;
    DungeonSpriteSheet m_sheet;
    AudioManager*    m_audio  = nullptr;
    bool             m_paused = false;

    // ── World dimensions (scene units) ────────────────────────────────────────
    static constexpr int WORLD_W = 800;
    static constexpr int WORLD_H = 600;
    static constexpr int SPEED   = 3;

    // ── Private helpers ───────────────────────────────────────────────────────
    void buildScene();
    void placePlayer();
    void spawnEnemies();
    void movePlayer();
    void patrolEnemies();
    void checkCollisions();
    void fitView();
    void togglePause();


    LevelDef            m_level;
    bool                m_bossTriggered = false;
    EnemySprite*        m_bossSprite    = nullptr;
};

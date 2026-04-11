#pragma once

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QTimer>
#include <QKeyEvent>
#include <QSet>
#include <QList>
#include "character.h"   // for CharacterType

// ─────────────────────────────────────────────────────────────────────────────
//  EnemySprite  –  a roaming enemy the player can bump into to start a battle.
// ─────────────────────────────────────────────────────────────────────────────
class EnemySprite : public QGraphicsRectItem
{
public:
    explicit EnemySprite(CharacterType type, const QString &name,
                         QGraphicsItem *parent = nullptr);

    static constexpr qreal W = 28;
    static constexpr qreal H = 28;

    CharacterType enemyType() const { return m_type; }
    QString       enemyName() const { return m_name; }

    // Patrol back-and-forth each frame, clamped to worldBounds.
    void patrol(const QRectF &worldBounds);

private:
    CharacterType m_type;
    QString       m_name;

    qreal m_vx;   // current horizontal velocity (pixels/frame)
    qreal m_vy;   // current vertical velocity
};

// ─────────────────────────────────────────────────────────────────────────────
//  DungeonWidget  –  dark dungeon exploration; touching an enemy starts a fight.
//
//  Signals
//  ───────
//    battleTriggered(CharacterType, QString)
//        Emitted when the player collides with an enemy.
//        Pass these straight to GameEngine::onPlayerSelectedCharacter() after
//        the player has chosen their own character (or store them so MainWindow
//        can forward them).
//
//    exitedDungeon()   player reached the exit portal back to the overworld
//    backToMenu()      player pressed Escape
// ─────────────────────────────────────────────────────────────────────────────
class DungeonWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DungeonWidget(QWidget *parent = nullptr);
    ~DungeonWidget() override = default;

    // Call every time this screen becomes visible.
    void activate();
    // Call when navigating away so the timer stops in the background.
    void deactivate();

signals:
    void battleTriggered(CharacterType enemyType, const QString &enemyName);
    void exitedDungeon();
    void backToMenu();

protected:
    void keyPressEvent  (QKeyEvent   *e) override;
    void keyReleaseEvent(QKeyEvent   *e) override;
    void resizeEvent    (QResizeEvent *e) override;

private slots:
    void onTick();

private:
    // ── Scene ────────────────────────────────────────────────────────────────
    QGraphicsScene    *m_scene     = nullptr;
    QGraphicsView     *m_view      = nullptr;

    // Player reuses the same PlayerSprite from the overworld concept but we
    // keep it self-contained here with a plain QGraphicsRectItem.
    QGraphicsRectItem *m_player    = nullptr;

    // Exit portal back to the overworld
    QGraphicsRectItem *m_exitZone  = nullptr;

    // Roaming enemies
    QList<EnemySprite *> m_enemies;

    // ── Game loop ────────────────────────────────────────────────────────────
    QTimer    m_ticker;
    QSet<int> m_heldKeys;

    // ── World dimensions ─────────────────────────────────────────────────────
    static constexpr int WORLD_W = 800;
    static constexpr int WORLD_H = 600;

    // ── Player size ──────────────────────────────────────────────────────────
    static constexpr qreal PW    = 28;
    static constexpr qreal PH    = 36;
    static constexpr int   SPEED = 3;

    // ── Helpers ──────────────────────────────────────────────────────────────
    void buildScene();
    void placePlayer();
    void spawnEnemies();
    void movePlayer();
    void patrolEnemies();
    void checkCollisions();
    void fitView();
};

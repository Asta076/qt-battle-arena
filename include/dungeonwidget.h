#pragma once

#include <QWidget>
#include <QGraphicsRectItem>
#include <QTimer>
#include <QSet>
#include <QList>

#include "character.h"
#include "playercontroller.h"

class QGraphicsView;
class QGraphicsScene;
class QResizeEvent;
class QKeyEvent;
class GoldHudWidget;
class AudioManager;
class PauseOverlayWidget;

class EnemySprite : public QGraphicsRectItem
{
public:
    explicit EnemySprite(CharacterType type, const QString& name,
                         QGraphicsItem* parent = nullptr);

    static constexpr qreal W = 28;
    static constexpr qreal H = 28;

    CharacterType enemyType() const { return m_type; }
    QString enemyName() const { return m_name; }

    void patrol(const QRectF& worldBounds);

private:
    CharacterType m_type;
    QString m_name;

    qreal m_vx;
    qreal m_vy;
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
    void battleTriggered(CharacterType enemyType, const QString& enemyName);
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
    QGraphicsRectItem* m_player = nullptr;
    QGraphicsRectItem* m_exitZone = nullptr;

    QList<EnemySprite*> m_enemies;

    QTimer m_ticker;
    QSet<int> m_heldKeys;

    static constexpr int WORLD_W = 800;
    static constexpr int WORLD_H = 600;

    static constexpr qreal PW = 28;
    static constexpr qreal PH = 36;
    static constexpr int SPEED = 3;

    void buildScene();
    void placePlayer();
    void spawnEnemies();
    void movePlayer();
    void patrolEnemies();
    void checkCollisions();
    void fitView();
    void togglePause();
    void buildPauseOverlay();

    AudioManager* m_audio = nullptr;
    PauseOverlayWidget* m_pauseOverlay = nullptr;
    bool m_paused = false;
};

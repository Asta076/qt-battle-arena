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
#include <QHash>
#include <QPixmap>

#include "character.h"
#include "enemy.h"
#include "worldcombatmanager.h"
#include "goldhudwidget.h"
#include "healthbarwidget.h"
#include "playercontroller.h"
#include "overworldwidget.h"

class QLabel;
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

struct DungeonAttackSheet
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
                                 const DungeonAttackSheet& attackSheet,
                                 QGraphicsItem* parent = nullptr);

    static constexpr qreal W = 96.0;
    static constexpr qreal H = 96.0;

    void updateAnimation(bool isMoving, Direction facingDir);
    void refreshFrame();

    void startAttackAnimation(Direction dir);
    bool isAttacking() const { return m_isAttacking; }

private:
    void setWalkAnim(Direction dir);
    void setIdleFrame(Direction dir);
    void applyFrame();
    void applyAttackFrame();

    const DungeonSpriteSheet& m_sheet;
    const DungeonAttackSheet& m_attackSheet;

    bool m_isIdle = true;
    bool m_isAttacking = false;

    Direction m_facing = Direction::Down;

    int m_frameIndex = 0;
    int m_tickAccum = 0;

    int m_attackFrameIndex = 0;
    int m_attackTickAccum = 0;

    static constexpr int TICKS_PER_FRAME = 5;
    static constexpr int WALK_FRAMES = 6;

    static constexpr int ATTACK_TICKS_PER_FRAME = 4;
    static constexpr int ATTACK_FRAMES = 7;

    QGraphicsEllipseItem* m_shadow = nullptr;
};

class EnemySprite : public QGraphicsPixmapItem
{
public:
    explicit EnemySprite(CharacterType type, const QString& name,
                         QGraphicsItem* parent = nullptr);

    static constexpr qreal W = 52;
    static constexpr qreal H = 52;

    CharacterType enemyType() const { return m_type; }
    QString enemyName() const { return m_name; }

    QRectF hitBox() const;

    void chasePlayer(const QRectF& playerBounds, const QRectF& worldBounds);
    void updateAnimation();

private:
    CharacterType m_type;
    QString m_name;

    QPixmap m_sprite;
    QGraphicsEllipseItem* m_shadow = nullptr;
    QGraphicsTextItem* m_nameLabel = nullptr;

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

    void setPlayerCharacterType(CharacterType type);
    void setPlayerCharacter(Character* player);

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
    DungeonAttackSheet m_attackSheet;

    CharacterType m_playerType = CharacterType::Archer;

    void buildScene();
    void placePlayer();
    void spawnEnemies();
    void clearEnemies();

    void movePlayer();
    void patrolEnemies();
    void checkCollisions();
    void fitView();
    void togglePause();

    void loadPlayerSheet(CharacterType type);
    void loadAttackSheet(CharacterType type);

    void handleClassAttack();
    void checkAttackCollisions();

    void buildPlayerHud();
    void updatePlayerHud();
    void positionPlayerHud();

    AudioManager* m_audio = nullptr;
    PauseOverlayWidget* m_pauseOverlay = nullptr;
    bool m_paused = false;

    QWidget* m_playerHud = nullptr;
    QLabel* m_healthLabel = nullptr;
    QLabel* m_specialLabel = nullptr;
    HealthBarWidget* m_healthBar = nullptr;
    HealthBarWidget* m_specialBar = nullptr;

    WorldCombatManager m_combat;
    QHash<EnemySprite*, Enemy*> m_enemyLogic;

    Direction m_facing = Direction::Down;
};
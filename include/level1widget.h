#pragma once

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QTimer>
#include <QSet>
#include <QHash>
#include <QList>

#include "dungeonwidget.h"      // DungeonPlayerSprite, EnemySprite, DungeonSpriteSheet
#include "enemy.h"
#include "worldcombatmanager.h"
#include "goldhudwidget.h"
#include "healthbarwidget.h"
#include "playercontroller.h"
#include "leveldef.h"
#include "playerprofile.h"
#include "spritecache.h"

class AudioManager;
class PauseOverlayWidget;
class QLabel;

class Level1Widget : public QWidget {
    Q_OBJECT

public:
    explicit Level1Widget(AudioManager* audio, QWidget* parent = nullptr);
    ~Level1Widget() override;

    void activate(const LevelDef& level, const PlayerProfile& profile);
    void deactivate();
    void reactivate();   // restart after returning from boss battle

    void setPlayerCharacterType(CharacterType type);
    void setPlayerCharacter(Character* player);

    // ── Enemy config — change these to retune per level ──────────────────────
    static constexpr int   ENEMY_COUNT        = 3;
    static constexpr int   ENEMY_HP           = 60;
    static constexpr int   ENEMY_DAMAGE       = 8;
    static constexpr int   BOSS_HP_MULTIPLIER = 4;   // 4× ENEMY_HP
    static constexpr qreal BOSS_SCALE         = 1.8; // visual size multiplier
    static constexpr int   BOSS_DAMAGE        = 18;

    static const QString ENEMY_MOVEMENT_SHEET;  // swap per level when new sprites arrive
    static const QString ENEMY_ATTACK_SHEET;

signals:
    void bossTriggered(const LevelDef& level);   // all regular enemies dead, boss reached
    void exitedLevel();                            // player walked to exit after boss
    void backToMenu();

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;

private slots:
    void onTick();

private:
    void buildScene();
    void placePlayer();
    void spawnEnemies();
    void clearEnemies();

    void movePlayer();
    void patrolEnemies();
    void checkCollisions();
    void checkAttackCollisions();
    void handleClassAttack();
    void handleSpecialAbility();

    void fitView();
    void togglePause();

    void loadPlayerSheet(CharacterType type);
    void loadAttackSheet(CharacterType type);

    void buildPlayerHud();
    void updatePlayerHud();
    void positionPlayerHud();

    static constexpr int WORLD_W = 800;
    static constexpr int WORLD_H = 600;

    QGraphicsScene*      m_scene       = nullptr;
    QGraphicsView*       m_view        = nullptr;
    DungeonPlayerSprite* m_player      = nullptr;
    GoldHudWidget*       m_goldHud     = nullptr;

    // Regular enemies
    QList<EnemySprite*>         m_enemies;
    QHash<EnemySprite*, Enemy*> m_enemyLogic;

    // Boss — separate from m_enemies, triggered when m_enemies is empty
    EnemySprite*  m_bossSprite  = nullptr;
    Enemy*        m_bossLogic   = nullptr;
    bool          m_bossActive  = false;   // true when all regular enemies dead
    bool          m_bossTriggered = false; // true after dialog fires

    QTimer           m_ticker;
    QSet<int>        m_heldKeys;
    Direction        m_facing    = Direction::Down;
    PlayerController m_controller;

    WorldCombatManager m_combat;

    LevelDef      m_level;

    DungeonSpriteSheet m_sheet;
    DungeonAttackSheet m_attackSheet;

    CharacterType m_playerType = CharacterType::Warrior;

    AudioManager*       m_audio        = nullptr;
    PauseOverlayWidget* m_pauseOverlay = nullptr;
    bool                m_paused       = false;

    QWidget*        m_playerHud    = nullptr;
    QLabel*         m_healthLabel  = nullptr;
    QLabel*         m_specialLabel = nullptr;
    HealthBarWidget* m_healthBar   = nullptr;
    HealthBarWidget* m_specialBar  = nullptr;

    QRectF m_exitArea;   // appears after boss is defeated
};
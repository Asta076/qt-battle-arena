#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include "gameengine.h"
#include "audiomanager.h"
#include "playerprofile.h"
#include "saveslotwidget.h"
#include "character.h"
#include "levelmanager.h"
#include "bossdialogwidget.h"
#include "storyslidedialog.h"       // ← NEW

class StartScreenWidget;
class CharacterSelectWidget;
class BattleWidget;
class GameOverWidget;
class ScoreboardWidget;
class OverworldWidget;
class DungeonWidget;
class HouseWidget;
class ShopWidget;
class Level1Widget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    // ── Engine ────────────────────────────────────────────────────────────────
    void onStateChanged(GameState newState);

    // ── Start screen ──────────────────────────────────────────────────────────
    void onStartRequested();
    void onLoadRequested();

    // ── Slot screen ───────────────────────────────────────────────────────────
    void onNewGameInSlot(int slotIndex);
    void onLoadSlot(int slotIndex);
    void onSlotBackRequested();

    // ── Exploration ───────────────────────────────────────────────────────────
    void onDungeonEntered();
    void onExitedDungeon();
    void onLevel1Entered();
    void onExitedLevel1();
    void onBattleTriggered(CharacterType enemyType, const QString& enemyName);
    void onBackToMenu();
    void onHouseEntered();
    void onHouseExited();
    void onShopEntered();
    void onShopExited();
    void onBuyItemRequested(ItemType type, int cost);

    // ── Profile events ────────────────────────────────────────────────────────
    void onGoldEarned(int amount);
    void onSaveRequested();
    void onReturnToOverworld();
    void onBattleItemChosen(ItemType type);

    // ── Boss dialog ───────────────────────────────────────────────────────────
    void onBossTriggered(const LevelDef& level);
    void onBossFightAccepted();
    void onBossOutroDismissed();

    // ── Story slide ───────────────────────────────────────────────────────────
    void onStoryFinished();             // ← NEW

private:
    void buildUI();
    void buildMenuBar();
    void updateGoldHud();

    // ── Core ──────────────────────────────────────────────────────────────────
    GameEngine*     m_engine           = nullptr;
    AudioManager*   m_audio            = nullptr;
    QStackedWidget* m_stack            = nullptr;
    PlayerProfile   m_profile;
    int             m_currentSlot      = -1;
    bool            m_playerHasChosen  = false;
    bool            m_hasPendingBattle = false;
    int  m_lastFinishedLevelId = 0;
    bool m_lastBossWon = false;
    CharacterType   m_pendingEnemyType = CharacterType::Warrior;
    QString         m_pendingEnemyName;

    // ── Screens ───────────────────────────────────────────────────────────────
    StartScreenWidget*     m_startScreen  = nullptr;
    SaveSlotWidget*        m_slotScreen   = nullptr;
    CharacterSelectWidget* m_charSelect   = nullptr;
    OverworldWidget*       m_overworld    = nullptr;
    DungeonWidget*         m_dungeon      = nullptr;
    Level1Widget*          m_level1       = nullptr;
    HouseWidget*           m_house        = nullptr;
    ShopWidget*            m_shop         = nullptr;
    BattleWidget*          m_battleWidget = nullptr;
    GameOverWidget*        m_gameOver     = nullptr;
    ScoreboardWidget*      m_scoreboard   = nullptr;

    // ── Level system ──────────────────────────────────────────────────────────
    LevelManager      m_levelManager;
    BossDialogWidget* m_bossDialog     = nullptr;
    StorySlideDialog* m_storySlide     = nullptr;   // ← NEW
    int               m_activeLevelId  = 0;
    int               m_pendingLevelId = 0;          // ← NEW

    enum class BattleOrigin { None, Dungeon, Level };
    BattleOrigin m_battleOrigin = BattleOrigin::None;
    bool         m_inLevel      = false;
};

#pragma once

#include <QMainWindow>
#include <QStackedWidget>

#include "gameengine.h"
#include "audiomanager.h"
#include "playerprofile.h"
#include "saveslotwidget.h"
#include "character.h"
#include "levelmanager.h"
#include "leveldef.h"

class StartScreenWidget;
class CharacterSelectWidget;
class BattleWidget;
class GameOverWidget;
class ScoreboardWidget;
class OverworldWidget;
class DungeonWidget;
class HouseWidget;
class ShopWidget;
class PvpArenaWidget;
class PvpCharacterSelectWidget;
class Level1Widget;
class LevelSelectWidget;
class StorySlideDialog;
class BossDialogWidget;

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
    void onPvpRequested();
    void onPvpDuelStartRequested(CharacterType p1Type,
                                 CharacterType p2Type,
                                 int roundsToWin);
    void onPvpBackToMenu();
    void onLoadRequested();

    // ── Slot screen ───────────────────────────────────────────────────────────
    void onNewGameInSlot(int slotIndex);
    void onLoadSlot(int slotIndex);
    void onSlotBackRequested();

    // ── Exploration ───────────────────────────────────────────────────────────
    void onDungeonEntered();
    void onExitedDungeon();
    void onBackToMenu();
    void onHouseEntered();
    void onHouseExited();
    void onShopEntered();
    void onShopExited();
    void onBuyItemRequested(ItemType type, int cost);

    // ── Profile events ────────────────────────────────────────────────────────
    void onSaveRequested();

    // ── Old battle item system ────────────────────────────────────────────────
    void onBattleItemChosen(ItemType type);

    // ── Level system ──────────────────────────────────────────────────────────
    void onLevelsEntered();
    void onLevelSelected(int levelId);
    void onStoryFinished();
    void onBossTriggered(const LevelDef& level);
    void onBossFightAccepted();
    void onBossOutroDismissed();
    void onExitedLevel();

private:
    void buildUI();
    void buildMenuBar();
    void updateGoldHud();

    // ── Core ──────────────────────────────────────────────────────────────────
    GameEngine*     m_engine          = nullptr;
    AudioManager*   m_audio           = nullptr;
    QStackedWidget* m_stack           = nullptr;
    PlayerProfile   m_profile;
    int             m_currentSlot     = -1;
    bool            m_playerHasChosen = false;
    bool            m_hasPendingBattle = false;

    // ── Screens ───────────────────────────────────────────────────────────────
    StartScreenWidget*        m_startScreen   = nullptr;
    SaveSlotWidget*           m_slotScreen    = nullptr;
    CharacterSelectWidget*    m_charSelect    = nullptr;
    OverworldWidget*          m_overworld     = nullptr;
    DungeonWidget*            m_dungeon       = nullptr;
    HouseWidget*              m_house         = nullptr;
    ShopWidget*               m_shop          = nullptr;
    BattleWidget*             m_battleWidget  = nullptr;
    GameOverWidget*           m_gameOver      = nullptr;
    ScoreboardWidget*         m_scoreboard    = nullptr;
    PvpArenaWidget*           m_pvpArena      = nullptr;
    PvpCharacterSelectWidget* m_pvpCharSelect = nullptr;

    Level1Widget*             m_level1        = nullptr;
    LevelSelectWidget*        m_levelSelect   = nullptr;
    StorySlideDialog*         m_storyDialog   = nullptr;
    BossDialogWidget*         m_bossDialog    = nullptr;

    LevelManager              m_levelManager;
    int                       m_activeLevelId = 0;
    bool                      m_inLevel       = false;

    enum class BattleOrigin { None, Dungeon, Level };
    BattleOrigin              m_battleOrigin  = BattleOrigin::None;
};
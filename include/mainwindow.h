#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include "gameengine.h"
#include "audiomanager.h"
#include "playerprofile.h"
#include "saveslotwidget.h"
#include "character.h"

class StartScreenWidget;
class CharacterSelectWidget;
class GameOverWidget;
class ScoreboardWidget;
class OverworldWidget;
class DungeonWidget;
class HouseWidget;
class ShopWidget;

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
    void onBattleTriggered(CharacterType enemyType, const QString& enemyName);
    void onBackToMenu();
    void onHouseEntered();
    void onHouseExited();
    void onShopEntered();
    void onShopExited();
    void onBuyItemRequested(ItemType type, int cost);

    // ── Profile events ────────────────────────────────────────────────────────
    void onGoldEarned(int amount);
    void onSaveRequested();       // from overworld pause SAVE button
    void onReturnToOverworld();   // from game over EXPLORE MORE

    //
    void onBattleItemChosen(ItemType type);
private:
    void buildUI();
    void buildMenuBar();
    void updateGoldHud();
    // ── Core ──────────────────────────────────────────────────────────────────
    GameEngine*     m_engine          = nullptr;
    AudioManager*   m_audio           = nullptr;
    QStackedWidget* m_stack           = nullptr;
    PlayerProfile   m_profile;
    int             m_currentSlot     = -1;     // active save slot (-1 = none)
    bool            m_playerHasChosen = false;
    bool            m_hasPendingBattle= false;  // enemy waiting after charSelect

    CharacterType   m_pendingEnemyType = CharacterType::Warrior;
    QString         m_pendingEnemyName;

    // ── Screens ───────────────────────────────────────────────────────────────
    StartScreenWidget*     m_startScreen  = nullptr;
    SaveSlotWidget*        m_slotScreen   = nullptr;
    CharacterSelectWidget* m_charSelect   = nullptr;
    OverworldWidget*       m_overworld    = nullptr;
    DungeonWidget*         m_dungeon      = nullptr;
    HouseWidget*           m_house        = nullptr;
    ShopWidget*            m_shop         = nullptr;
    GameOverWidget*        m_gameOver     = nullptr;
    ScoreboardWidget*      m_scoreboard   = nullptr;
};

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
class OverworldWidget;
class DungeonWidget;
class HouseWidget;
class ShopWidget;
class PvpArenaWidget;
class PvpCharacterSelectWidget;

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
    void onPvpDuelStartRequested(CharacterType p1Type, CharacterType p2Type);
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

private:
    void buildUI();
    void buildMenuBar();
    void updateGoldHud();
    // ── Core ──────────────────────────────────────────────────────────────────
    GameEngine*     m_engine      = nullptr;
    AudioManager*   m_audio       = nullptr;
    QStackedWidget* m_stack       = nullptr;
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
    PvpArenaWidget*        m_pvpArena     = nullptr;
    PvpCharacterSelectWidget* m_pvpCharSelect = nullptr;
};

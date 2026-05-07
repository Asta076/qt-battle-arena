#include "mainwindow.h"
#include "startscreenwidget.h"
#include "saveslotwidget.h"
#include "characterselectiondialog.h"
#include "battlewidget.h"
#include "gameoverwidget.h"
#include "scoreboardwidget.h"
#include "overworldwidget.h"
#include "dungeonwidget.h"
#include "shopwidget.h"
#include "housewidget.h"
#include "level1widget.h"      // ← NEW
#include "audiomanager.h"
#include <QMenuBar>
#include <QAction>
#include <QApplication>
#include <QFile>

// ── Item effect constants ────────
static constexpr int HEALTH_POTION_AMOUNT = 0;   // 0 = use 35% of max HP
static constexpr int SP_POTION_AMOUNT     = 50;


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    m_engine = new GameEngine(this);
    buildUI();
    buildMenuBar();

    connect(m_engine, &GameEngine::stateChanged,
            this, &MainWindow::onStateChanged);
    connect(m_engine, &GameEngine::goldEarned,
            this, &MainWindow::onGoldEarned);

    m_audio->playMusic("/music/menu.ogg");
}

MainWindow::~MainWindow() = default;

// ─────────────────────────────────────────────────────────────────────────────
//  Build
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::buildUI()
{
    m_audio = new AudioManager(this);
    m_stack = new QStackedWidget(this);

    m_startScreen  = new StartScreenWidget(m_engine, this);
    m_slotScreen   = new SaveSlotWidget(this);
    m_charSelect   = new CharacterSelectWidget(m_engine, this);
    m_overworld    = new OverworldWidget(m_audio, this);
    m_dungeon      = new DungeonWidget(m_audio, this);
    m_level1       = new Level1Widget(m_audio, this);   // ← NEW
    m_house        = new HouseWidget(m_audio, this);
    m_shop         = new ShopWidget(this);
    m_battleWidget = new BattleWidget(m_engine, m_audio, &m_profile, this);
    m_gameOver     = new GameOverWidget(m_engine, this);
    m_scoreboard   = new ScoreboardWidget(m_engine, this);

    m_stack->addWidget(m_startScreen);
    m_stack->addWidget(m_slotScreen);
    m_stack->addWidget(m_charSelect);
    m_stack->addWidget(m_overworld);
    m_stack->addWidget(m_dungeon);
    m_stack->addWidget(m_level1);
    m_stack->addWidget(m_house);
    m_stack->addWidget(m_shop);
    m_stack->addWidget(m_battleWidget);
    m_stack->addWidget(m_gameOver);
    m_stack->addWidget(m_scoreboard);

    setCentralWidget(m_stack);
    m_stack->setCurrentWidget(m_startScreen);

    // ── Start screen ─────────────────────────────────────────────────────────
    connect(m_startScreen, &StartScreenWidget::startRequested,
            this, &MainWindow::onStartRequested);
    connect(m_startScreen, &StartScreenWidget::loadRequested,
            this, &MainWindow::onLoadRequested);

    // ── Slot screen ──────────────────────────────────────────────────────────
    connect(m_slotScreen, &SaveSlotWidget::newGameRequested,
            this, &MainWindow::onNewGameInSlot);
    connect(m_slotScreen, &SaveSlotWidget::loadRequested,
            this, &MainWindow::onLoadSlot);
    connect(m_slotScreen, &SaveSlotWidget::backRequested,
            this, &MainWindow::onSlotBackRequested);

    // ── Overworld ────────────────────────────────────────────────────────────
    connect(m_overworld, &OverworldWidget::dungeonEntered,
            this, &MainWindow::onDungeonEntered);
    connect(m_overworld, &OverworldWidget::level1Entered,
            this, &MainWindow::onLevel1Entered);          // ← NEW
    connect(m_overworld, &OverworldWidget::backToMenu,
            this, &MainWindow::onBackToMenu);
    connect(m_overworld, &OverworldWidget::saveRequested,
            this, &MainWindow::onSaveRequested);


    // ── House ──────────────────────────────────────────────────────────────

    connect(m_overworld, &OverworldWidget::houseEntered,
            this, &MainWindow::onHouseEntered);
    connect(m_house, &HouseWidget::backToOverworld,
            this, &MainWindow::onHouseExited);

    // ── Dungeon ──────────────────────────────────────────────────────────────
    connect(m_dungeon, &DungeonWidget::battleTriggered,
            this, &MainWindow::onBattleTriggered);
    connect(m_dungeon, &DungeonWidget::exitedDungeon,
            this, &MainWindow::onExitedDungeon);
    connect(m_dungeon, &DungeonWidget::backToMenu,
            this, &MainWindow::onBackToMenu);

    // ── Level 1 ──────────────────────────────────────────────────────────────
    connect(m_level1, &Level1Widget::battleTriggered,
            this, &MainWindow::onBattleTriggered);
    connect(m_level1, &Level1Widget::exitedLevel,
            this, &MainWindow::onExitedLevel1);
    connect(m_level1, &Level1Widget::backToMenu,
            this, &MainWindow::onBackToMenu);

    // ── Boss dialog overlay ───────────────────────────────────────────────────
    m_bossDialog = new BossDialogWidget(this);
    m_bossDialog->hide();

    connect(m_level1, &Level1Widget::bossTriggered,
            this, &MainWindow::onBossTriggered);
    connect(m_bossDialog, &BossDialogWidget::fightAccepted,
            this, &MainWindow::onBossFightAccepted);
    connect(m_bossDialog, &BossDialogWidget::dialogDismissed,
            this, &MainWindow::onBossOutroDismissed);

    // --Shop---------------------------------------------------------------
    connect(m_overworld, &OverworldWidget::shopEntered,
            this, &MainWindow::onShopEntered);
    connect(m_shop, &ShopWidget::backToOverworld,
            this, &MainWindow::onShopExited);
    connect(m_shop, &ShopWidget::buyItemRequested,
            this, &MainWindow::onBuyItemRequested);

    // ── Game over ─────────────────────────────────────────────────────────────
    connect(m_gameOver, &GameOverWidget::returnToOverworld,
            this, &MainWindow::onReturnToOverworld);

    connect(m_battleWidget, &BattleWidget::itemChosen,
            this, &MainWindow::onBattleItemChosen);
}

void MainWindow::buildMenuBar()
{
    QMenu* gameMenu = menuBar()->addMenu("Game");
    QAction* pauseAct = gameMenu->addAction("Pause");
    connect(pauseAct, &QAction::triggered, m_engine, &GameEngine::onPauseToggle);
    gameMenu->addSeparator();
    QAction* exitAct = gameMenu->addAction("Exit");
    connect(exitAct, &QAction::triggered, qApp, &QApplication::quit);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Start screen → slot screen
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onStartRequested()
{
    m_slotScreen->refresh(SlotMode::NewGame);
    m_stack->setCurrentWidget(m_slotScreen);
}

void MainWindow::onLoadRequested()
{
    m_slotScreen->refresh(SlotMode::Load);
    m_stack->setCurrentWidget(m_slotScreen);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Slot screen
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onNewGameInSlot(int slotIndex)
{
    QFile::remove(SaveSlotWidget::slotPath(slotIndex));

    m_currentSlot      = slotIndex;
    m_profile.reset();

    // give the player their starting items
    m_profile.addItem(ItemType::HealthPotion, 5);

    m_playerHasChosen  = false;
    m_hasPendingBattle = false;

    m_engine->onRestartGame();   // reset engine to clean state silently
    // Go to char select — engine emits no state change yet
    m_stack->setCurrentWidget(m_charSelect);
}

void MainWindow::onLoadSlot(int slotIndex)
{
    m_currentSlot = slotIndex;
    m_profile.loadFromFile(SaveSlotWidget::slotPath(slotIndex));

    m_playerHasChosen  = true;
    m_hasPendingBattle = false;

    // Tell the engine who the player is so future battles use the right class
    m_engine->setPlayerIdentity(
        static_cast<CharacterType>(m_profile.characterType),
        m_profile.characterName);

    m_engine->setStatBonuses(
        m_profile.upgrades.bonusMaxHp,
        m_profile.upgrades.bonusAttack,
        m_profile.upgrades.bonusSpPerAtk);

    updateGoldHud();
    m_overworld->activate();
    m_stack->setCurrentWidget(m_overworld);
}

void MainWindow::onSlotBackRequested()
{
    m_stack->setCurrentWidget(m_startScreen);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Engine state → screen routing
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onStateChanged(GameState newState)
{
    switch (newState) {

    case GameState::MainMenu:
        m_stack->setCurrentWidget(m_startScreen);
        m_audio->playMusic("/music/menu.ogg");
        break;

    case GameState::CharacterSelect:
        // Handled directly by onNewGameInSlot — guard only
        m_stack->setCurrentWidget(m_charSelect);
        break;

    case GameState::PlayerTurn:
        if (m_stack->currentWidget() == m_charSelect) {
            // Player just confirmed their character
            m_playerHasChosen = true;

            // Save identity to profile immediately
            m_profile.characterName = m_engine->getPlayerName();
            m_profile.characterType = static_cast<int>(m_engine->getPlayerType());

            if (m_hasPendingBattle) {
                // Was sent to charSelect because of dungeon collision
                m_hasPendingBattle = false;
                m_engine->setStatBonuses(
                    m_profile.upgrades.bonusMaxHp,
                    m_profile.upgrades.bonusAttack,
                    m_profile.upgrades.bonusSpPerAtk);
                m_stack->setCurrentWidget(m_battleWidget);
                m_audio->playMusic("/music/battle.ogg");
            } else {
                m_engine->setStatBonuses(0, 0, 0);
                // Normal new-game flow — save initial profile and go to overworld
                if (m_currentSlot >= 0)
                    m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));
                m_overworld->activate();
                m_stack->setCurrentWidget(m_overworld);
            }
        } else {
            // Mid-battle: back to player's turn
            m_stack->setCurrentWidget(m_battleWidget);
            m_audio->playMusic("/music/battle.ogg");
        }
        break;

    case GameState::Playing:
    case GameState::AnimatingAttack:
    case GameState::Paused:
    case GameState::RoundOver:
        m_stack->setCurrentWidget(m_battleWidget);
        break;

    case GameState::GameOver:
        m_audio->stopMusic();

        if (m_activeLevelId > 0) {
            const LevelDef* lvl = m_levelManager.level(m_activeLevelId);
            bool playerWon = m_engine->getPlayerScore() > m_engine->getEnemyScore();

            if (playerWon && lvl) {
                m_levelManager.completeLevel(m_activeLevelId, m_profile);
                if (m_currentSlot >= 0)
                    m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));
                updateGoldHud();
            }

            m_activeLevelId = 0;

            if (lvl) {
                m_stack->setCurrentWidget(m_level1);
                m_bossDialog->setParent(m_level1);
                m_bossDialog->resize(m_level1->size());
                m_bossDialog->showOutro(*lvl, playerWon);
                m_bossDialog->show();
                m_bossDialog->raise();
                m_bossDialog->setFocus();
            }
        } else {
            m_stack->setCurrentWidget(m_gameOver);
        }
        break;

    case GameState::Scoreboard:
        m_stack->setCurrentWidget(m_scoreboard);
        break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Exploration
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onDungeonEntered()
{
    m_dungeon->activate();
    m_stack->setCurrentWidget(m_dungeon);
    // Dungeon's activate() calls playMusic("/music/battle.ogg") internally
}

void MainWindow::onExitedDungeon()
{
    // Clean exit via portal — count the run and auto-save
    m_profile.dungeonRuns++;
    if (m_currentSlot >= 0)
        m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));

    updateGoldHud();
    m_overworld->activate();
    m_stack->setCurrentWidget(m_overworld);
}

void MainWindow::onLevel1Entered()
{
    const LevelDef* lvl = m_levelManager.level(1);
    if (!lvl) return;

    if (!m_levelManager.isUnlocked(1, m_profile)) {
        // Show a brief message and return — don't enter
        // For now just do nothing; the overworld zone hint text explains it
        m_overworld->activate();
        return;
    }

    m_activeLevelId = 1;
    m_level1->activate(*lvl, m_profile);   // ← pass level + profile
    m_stack->setCurrentWidget(m_level1);
}

void MainWindow::onExitedLevel1()
{
    updateGoldHud();
    m_overworld->activate();
    m_stack->setCurrentWidget(m_overworld);
}

void MainWindow::onHouseEntered()
{
    m_house->setProfile(&m_profile);
    m_stack->setCurrentWidget(m_house);
}

void MainWindow::onHouseExited()
{
    m_overworld->activate();
    m_stack->setCurrentWidget(m_overworld);
}
void MainWindow::onShopEntered()
{
    m_shop->setProfile(&m_profile);
    m_stack->setCurrentWidget(m_shop);
}

void MainWindow::onShopExited()
{
    m_overworld->activate();
    m_stack->setCurrentWidget(m_overworld);
}
void MainWindow::onBattleTriggered(CharacterType enemyType, const QString& enemyName)
{
    m_pendingEnemyType = enemyType;
    m_pendingEnemyName = enemyName;

    if (!m_playerHasChosen) {
        // First encounter — need character select first
        m_hasPendingBattle = true;
        m_stack->setCurrentWidget(m_charSelect);
    } else {
        m_engine->setStatBonuses(
            m_profile.upgrades.bonusMaxHp,
            m_profile.upgrades.bonusAttack,
            m_profile.upgrades.bonusSpPerAtk);
        // Character already chosen — jump straight to battle
        m_engine->onPlayerSelectedCharacter(
            m_engine->getPlayerType(),
            m_engine->getPlayerName());
        // Engine emits PlayerTurn → onStateChanged shows battleWidget
    }
}

void MainWindow::onBackToMenu()
{
    if (m_currentSlot >= 0)
        m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));
    m_playerHasChosen  = false;
    m_hasPendingBattle = false;
    m_currentSlot      = -1;
    m_profile.reset();
    m_engine->onRestartGame();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Profile events
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onGoldEarned(int amount)
{
    // Accumulate in memory — written to disk only on dungeon exit or manual save.
    // Gold from an incomplete dungeon run is intentionally lost on quit.
    m_profile.addGold(amount);
    updateGoldHud();
}

void MainWindow::onSaveRequested()
{
    // Called by overworld pause SAVE button
    if (m_currentSlot < 0) return;
    m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));
}

void MainWindow::onReturnToOverworld()
{
    // Game over → "EXPLORE MORE": save gold earned this run then go to overworld
    m_profile.dungeonRuns++;
    if (m_currentSlot >= 0)
        m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));
    updateGoldHud();
    m_overworld->activate();
    m_stack->setCurrentWidget(m_overworld);
}

void MainWindow::onBuyItemRequested(ItemType type, int cost)
{
    // Guard: can the player still afford it? (shop already checks, but be safe)
    if (m_profile.gold < cost) return;

    // Deduct gold and add item
    m_profile.spendGold(cost);
    m_profile.addItem(type, 1);

    // Persist immediately — buying is a save-worthy event
    if (m_currentSlot >= 0)
        m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));

    // Tell the shop to refresh its display (gold changed, owned count changed)
    m_shop->setProfile(&m_profile);

    // Keep gold HUD in sync for when the player returns to the overworld
    updateGoldHud();
}

void MainWindow::updateGoldHud()
{
    m_overworld->setGold(m_profile.gold);
    m_dungeon->setGold(m_profile.gold);
}

void MainWindow::onBattleItemChosen(ItemType type)
{
    // Guard: make sure player actually has this item
    if (!m_profile.hasItem(type)) return;

    // Deduct from inventory first
    m_profile.removeItem(type, 1);

    // Then call the appropriate engine effect
    switch (type) {
    case ItemType::HealthPotion: {
        // 35% of player's max HP — engine calculates internally
        int maxHp = 0;
        const auto& chars = m_engine->getAllCharacters();
        if (!chars.isEmpty()) maxHp = chars[0]->getMaxHealth();
        int healAmt = static_cast<int>(maxHp * 0.35f);
        m_engine->onPlayerHealed(healAmt);
        break;
    }
    case ItemType::SpPotion:
        m_engine->onPlayerSpRestored(SP_POTION_AMOUNT);
        break;
    case ItemType::AttackBoost:
        m_engine->onPlayerAttackBoosted();
        break;
    case ItemType::DefenseBoost:
        m_engine->onPlayerDefenseActivated();
        break;
    }
}

void MainWindow::onBossTriggered(const LevelDef& level)
{
    // Level ticker already stopped inside Level1Widget
    m_bossDialog->resize(m_level1->size());
    m_bossDialog->setParent(m_level1);
    m_bossDialog->showIntro(level, m_profile.characterName);
    m_bossDialog->show();
    m_bossDialog->raise();
    m_bossDialog->setFocus();
}

void MainWindow::onBossFightAccepted()
{
    m_bossDialog->hide();

    const LevelDef* lvl = m_levelManager.level(m_activeLevelId);
    if (!lvl) return;

    CharacterType bossType = (m_activeLevelId == 5)
                                 ? static_cast<CharacterType>(m_profile.characterType)
                                 : lvl->bossType;

    m_pendingEnemyType = bossType;
    m_pendingEnemyName = lvl->bossName;

    m_engine->setStatBonuses(
        m_profile.upgrades.bonusMaxHp,
        m_profile.upgrades.bonusAttack,
        m_profile.upgrades.bonusSpPerAtk);

    m_engine->onPlayerSelectedCharacter(
        static_cast<CharacterType>(m_profile.characterType),
        m_profile.characterName);
    // Engine emits PlayerTurn → onStateChanged → battleWidget
}

void MainWindow::onBossOutroDismissed()
{
    m_bossDialog->hide();
    m_level1->deactivate();
    updateGoldHud();
    m_overworld->activate();
    m_stack->setCurrentWidget(m_overworld);
}

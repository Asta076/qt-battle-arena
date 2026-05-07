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
#include "level1widget.h"
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
    m_level1       = new Level1Widget(m_audio, this);
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
            this, &MainWindow::onLevel1Entered);
    connect(m_overworld, &OverworldWidget::backToMenu,
            this, &MainWindow::onBackToMenu);
    connect(m_overworld, &OverworldWidget::saveRequested,
            this, &MainWindow::onSaveRequested);

    // ── House ─────────────────────────────────────────────────────────────────
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

    // ── Story slide overlay ───────────────────────────────────────────────────
    // Parented to MainWindow so it covers the whole window
    m_storySlide = new StorySlideDialog(this);
    m_storySlide->hide();

    connect(m_storySlide, &StorySlideDialog::finished,
            this, &MainWindow::onStoryFinished);             // ← NEW

    // ── Shop ──────────────────────────────────────────────────────────────────
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

    m_engine->onRestartGame();
    m_stack->setCurrentWidget(m_charSelect);
}

void MainWindow::onLoadSlot(int slotIndex)
{
    m_currentSlot = slotIndex;
    m_profile.loadFromFile(SaveSlotWidget::slotPath(slotIndex));

    m_playerHasChosen  = true;
    m_hasPendingBattle = false;

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
        m_stack->setCurrentWidget(m_charSelect);
        break;

    case GameState::PlayerTurn:
        if (m_stack->currentWidget() == m_charSelect) {
            m_playerHasChosen = true;

            m_profile.characterName = m_engine->getPlayerName();
            m_profile.characterType = static_cast<int>(m_engine->getPlayerType());

            if (m_hasPendingBattle) {
                m_hasPendingBattle = false;
                m_engine->setStatBonuses(
                    m_profile.upgrades.bonusMaxHp,
                    m_profile.upgrades.bonusAttack,
                    m_profile.upgrades.bonusSpPerAtk);
                m_stack->setCurrentWidget(m_battleWidget);
                m_audio->playMusic("/music/battle.ogg");
            } else {
                m_engine->setStatBonuses(0, 0, 0);
                if (m_currentSlot >= 0)
                    m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));
                m_overworld->activate();
                m_stack->setCurrentWidget(m_overworld);
            }
        } else {
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
            // Boss fight just ended
            const LevelDef* lvl = m_levelManager.level(m_activeLevelId);
            bool playerWon = m_engine->getPlayerScore() > m_engine->getEnemyScore();

            if (playerWon && lvl) {
                m_levelManager.completeLevel(m_activeLevelId, m_profile);
                if (m_currentSlot >= 0)
                    m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));
                updateGoldHud();
            }

            int levelId     = m_activeLevelId;
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
}

void MainWindow::onExitedDungeon()
{
    m_battleOrigin = BattleOrigin::None;
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
        m_overworld->activate();
        return;
    }

    m_inLevel       = true;
    m_activeLevelId = 0;
    m_pendingLevelId = 1;   // ← remember which level to activate after story

    // ── Show story slide first if pages exist ─────────────────────────────────
    if (!lvl->storyPages.isEmpty()) {
        m_storySlide->resize(size());
        m_storySlide->show(lvl->name, lvl->storyPages, lvl->enterPrompt);
        m_storySlide->raise();
    } else {
        // No story — activate immediately
        m_level1->activate(*lvl, m_profile);
        m_stack->setCurrentWidget(m_level1);
    }
}

// ── NEW: fires when player presses the enter button on the last story page ──
void MainWindow::onStoryFinished()
{
    m_storySlide->hide();

    const LevelDef* lvl = m_levelManager.level(m_pendingLevelId);
    if (!lvl) return;

    m_level1->activate(*lvl, m_profile);
    m_stack->setCurrentWidget(m_level1);
}

void MainWindow::onExitedLevel1()
{
    m_inLevel = false;
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

    m_battleOrigin = m_inLevel ? BattleOrigin::Level : BattleOrigin::Dungeon;

    if (!m_playerHasChosen) {
        m_hasPendingBattle = true;
        m_stack->setCurrentWidget(m_charSelect);
    } else {
        m_engine->setStatBonuses(
            m_profile.upgrades.bonusMaxHp,
            m_profile.upgrades.bonusAttack,
            m_profile.upgrades.bonusSpPerAtk);
        m_engine->onPlayerSelectedCharacter(
            m_engine->getPlayerType(),
            m_engine->getPlayerName());
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
    m_profile.addGold(amount);
    updateGoldHud();
}

void MainWindow::onSaveRequested()
{
    if (m_currentSlot < 0) return;
    m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));
}

void MainWindow::onReturnToOverworld()
{
    if (m_currentSlot >= 0)
        m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));
    updateGoldHud();

    if (m_battleOrigin == BattleOrigin::Dungeon) {
        m_battleOrigin = BattleOrigin::None;
        m_dungeon->reactivate();
        m_stack->setCurrentWidget(m_dungeon);
    } else if (m_battleOrigin == BattleOrigin::Level && m_inLevel) {
        m_battleOrigin = BattleOrigin::None;
        m_level1->reactivate();
        m_stack->setCurrentWidget(m_level1);
    } else {
        m_battleOrigin = BattleOrigin::None;
        m_inLevel      = false;
        m_overworld->activate();
        m_stack->setCurrentWidget(m_overworld);
    }
}

void MainWindow::onBuyItemRequested(ItemType type, int cost)
{
    if (m_profile.gold < cost) return;

    m_profile.spendGold(cost);
    m_profile.addItem(type, 1);

    if (m_currentSlot >= 0)
        m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));

    m_shop->setProfile(&m_profile);
    updateGoldHud();
}

void MainWindow::updateGoldHud()
{
    m_overworld->setGold(m_profile.gold);
    m_dungeon->setGold(m_profile.gold);
    m_level1->setGold(m_profile.gold);
}

void MainWindow::onBattleItemChosen(ItemType type)
{
    if (!m_profile.hasItem(type)) return;

    m_profile.removeItem(type, 1);

    switch (type) {
    case ItemType::HealthPotion: {
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

// ─────────────────────────────────────────────────────────────────────────────
//  Boss dialog
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onBossTriggered(const LevelDef& level)
{
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
    m_activeLevelId = m_inLevel ? 1 : 0;
    m_battleOrigin  = BattleOrigin::Level;

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
}

void MainWindow::onBossOutroDismissed()
{
    m_bossDialog->hide();
    m_level1->deactivate();
    m_inLevel = false;
    updateGoldHud();
    m_overworld->activate();
    m_stack->setCurrentWidget(m_overworld);
}

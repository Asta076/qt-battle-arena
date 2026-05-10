#include "mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QFile>
#include <QMenu>
#include <QMenuBar>

#include "audiomanager.h"
#include "characterselectiondialog.h"
#include "dungeonwidget.h"
#include "gameoverwidget.h"
#include "housewidget.h"
#include "overworldwidget.h"
#include "pvpcharacterselectwidget.h"
#include "pvparenawidget.h"
#include "saveslotwidget.h"
#include "shopwidget.h"
#include "startscreenwidget.h"
#include "level1widget.h"
#include "levelselectwidget.h"
#include "storyslidedialog.h"
#include "bossdialogwidget.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    m_engine = new GameEngine(this);

    buildUI();
    buildMenuBar();

    connect(m_engine, &GameEngine::stateChanged,
            this, &MainWindow::onStateChanged);

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

    m_startScreen   = new StartScreenWidget(m_engine, this);
    m_slotScreen    = new SaveSlotWidget(this);
    m_charSelect    = new CharacterSelectWidget(m_engine, this);
    m_overworld     = new OverworldWidget(m_audio, this);
    m_dungeon       = new DungeonWidget(m_audio, this);
    m_house         = new HouseWidget(m_audio, this);
    m_shop          = new ShopWidget(this);
    m_gameOver      = new GameOverWidget(m_engine, this);
    m_pvpCharSelect = new PvpCharacterSelectWidget(this);
    m_pvpArena      = new PvpArenaWidget(this);

    m_level1      = new Level1Widget(m_audio, this);
    m_levelSelect = new LevelSelectWidget(&m_levelManager, this);
    m_storyDialog = new StorySlideDialog(this);
    m_bossDialog  = new BossDialogWidget(this);
    m_storyDialog->hide();
    m_bossDialog->hide();

    m_stack->addWidget(m_startScreen);
    m_stack->addWidget(m_slotScreen);
    m_stack->addWidget(m_charSelect);
    m_stack->addWidget(m_overworld);
    m_stack->addWidget(m_dungeon);
    m_stack->addWidget(m_house);
    m_stack->addWidget(m_shop);
    m_stack->addWidget(m_gameOver);
    m_stack->addWidget(m_pvpCharSelect);
    m_stack->addWidget(m_pvpArena);
    m_stack->addWidget(m_level1);
    m_stack->addWidget(m_levelSelect);

    setCentralWidget(m_stack);
    m_stack->setCurrentWidget(m_startScreen);

    // ── Start screen ─────────────────────────────────────────────────────────
    connect(m_startScreen, &StartScreenWidget::startRequested,
            this, &MainWindow::onStartRequested);

    connect(m_startScreen, &StartScreenWidget::pvpRequested,
            this, &MainWindow::onPvpRequested);

    connect(m_startScreen, &StartScreenWidget::loadRequested,
            this, &MainWindow::onLoadRequested);

    // ── PvP ──────────────────────────────────────────────────────────────────
    connect(m_pvpArena, &PvpArenaWidget::backToMenu,
            this, &MainWindow::onPvpBackToMenu);

    connect(m_pvpCharSelect, &PvpCharacterSelectWidget::duelStartRequested,
            this, &MainWindow::onPvpDuelStartRequested);

    connect(m_pvpCharSelect, &PvpCharacterSelectWidget::backRequested,
            this, &MainWindow::onPvpBackToMenu);

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

    connect(m_overworld, &OverworldWidget::backToMenu,
            this, &MainWindow::onBackToMenu);

    connect(m_overworld, &OverworldWidget::saveRequested,
            this, &MainWindow::onSaveRequested);

    connect(m_overworld, &OverworldWidget::houseEntered,
            this, &MainWindow::onHouseEntered);

    connect(m_overworld, &OverworldWidget::shopEntered,
            this, &MainWindow::onShopEntered);

    connect(m_overworld, &OverworldWidget::levelsEntered,
            this, &MainWindow::onLevelsEntered);

    // ── Dungeon ──────────────────────────────────────────────────────────────
    connect(m_dungeon, &DungeonWidget::exitedDungeon,
            this, &MainWindow::onExitedDungeon);

    connect(m_dungeon, &DungeonWidget::backToMenu,
            this, &MainWindow::onBackToMenu);

    connect(m_dungeon, &DungeonWidget::goldEarned,
            this, &MainWindow::onGoldEarned);

    connect(m_dungeon, &DungeonWidget::dungeonGameOver,
            this, &MainWindow::onDungeonGameOver);

    // ── Level system ─────────────────────────────────────────────────────────
    connect(m_levelSelect, &LevelSelectWidget::levelSelected,
            this, &MainWindow::onLevelSelected);
    connect(m_levelSelect, &LevelSelectWidget::backRequested,
            this, [this]{ m_overworld->activate(); m_stack->setCurrentWidget(m_overworld); });

    connect(m_storyDialog, &StorySlideDialog::finished,
            this, &MainWindow::onStoryFinished);

    connect(m_level1, &Level1Widget::bossTriggered,
            this, &MainWindow::onBossTriggered);
    connect(m_level1, &Level1Widget::exitedLevel,
            this, &MainWindow::onExitedLevel);
    connect(m_level1, &Level1Widget::backToMenu,
            this, &MainWindow::onBackToMenu);

    connect(m_bossDialog, &BossDialogWidget::fightAccepted,
            this, &MainWindow::onBossFightAccepted);
    connect(m_bossDialog, &BossDialogWidget::dialogDismissed,
            this, &MainWindow::onBossOutroDismissed);

    // ── House ────────────────────────────────────────────────────────────────
    connect(m_house, &HouseWidget::backToOverworld,
            this, &MainWindow::onHouseExited);

    // ── Shop ─────────────────────────────────────────────────────────────────
    connect(m_shop, &ShopWidget::backToOverworld,
            this, &MainWindow::onShopExited);

    connect(m_shop, &ShopWidget::buyItemRequested,
            this, &MainWindow::onBuyItemRequested);

    // ── Dungeon game over ───────────────────────────────────────────────────
    connect(m_gameOver, &GameOverWidget::returnToOverworld,
            this, &MainWindow::onReturnToOverworld);

    connect(m_gameOver, &GameOverWidget::backToMenuRequested,
            this, &MainWindow::onBackToMenu);
}

void MainWindow::buildMenuBar()
{
    QMenu* gameMenu = menuBar()->addMenu("Game");

    QAction* pauseAct = gameMenu->addAction("Pause");
    connect(pauseAct, &QAction::triggered,
            m_engine, &GameEngine::onPauseToggle);

    gameMenu->addSeparator();

    QAction* exitAct = gameMenu->addAction("Exit");
    connect(exitAct, &QAction::triggered,
            qApp, &QApplication::quit);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Start screen
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onStartRequested()
{
    m_slotScreen->refresh(SlotMode::NewGame);
    m_stack->setCurrentWidget(m_slotScreen);
}

void MainWindow::onPvpRequested()
{
    m_stack->setCurrentWidget(m_pvpCharSelect);
}

void MainWindow::onPvpDuelStartRequested(CharacterType p1Type,
                                         CharacterType p2Type,
                                         int roundsToWin)
{
    m_pvpArena->setFighters(p1Type, p2Type);
    m_pvpArena->setMatchTarget(roundsToWin);

    m_stack->setCurrentWidget(m_pvpArena);
    m_pvpArena->activate();

    m_audio->playMusic("/music/battle.ogg");
}

void MainWindow::onPvpBackToMenu()
{
    if (m_pvpArena) {
        m_pvpArena->deactivate();
    }

    m_stack->setCurrentWidget(m_startScreen);
    m_audio->playMusic("/music/menu.ogg");
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

    m_currentSlot = slotIndex;
    m_profile.reset();
    m_profile.addItem(ItemType::HealthPotion, 5);

    m_playerHasChosen = false;
    m_hasPendingBattle = false;

    m_engine->onRestartGame();
    m_stack->setCurrentWidget(m_charSelect);
}

void MainWindow::onLoadSlot(int slotIndex)
{
    m_currentSlot = slotIndex;
    m_profile.loadFromFile(SaveSlotWidget::slotPath(slotIndex));

    m_playerHasChosen = true;
    m_hasPendingBattle = false;

    CharacterType loadedType = static_cast<CharacterType>(m_profile.characterType);

    m_engine->setPlayerIdentity(
        loadedType,
        m_profile.characterName
    );

    m_engine->setStatBonuses(
        m_profile.upgrades.bonusMaxHp,
        m_profile.upgrades.bonusAttack,
        m_profile.upgrades.bonusSpPerAtk
    );

    m_overworld->setPlayerCharacterType(loadedType);
    m_dungeon->setPlayerCharacterType(loadedType);

    updateGoldHud();

    m_overworld->activate();
    m_stack->setCurrentWidget(m_overworld);
}

void MainWindow::onSlotBackRequested()
{
    m_stack->setCurrentWidget(m_startScreen);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Engine state routing
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onStateChanged(GameState newState)
{
    switch (newState)
    {
    case GameState::MainMenu:
        m_stack->setCurrentWidget(m_startScreen);
        m_audio->playMusic("/music/menu.ogg");
        break;

    case GameState::CharacterSelect:
        m_stack->setCurrentWidget(m_charSelect);
        break;

    case GameState::Playing:
        if (!m_playerHasChosen) {
            m_profile.characterName = m_engine->getPlayerName();
            m_profile.characterType = static_cast<int>(m_engine->getPlayerType());
            m_playerHasChosen = true;

            CharacterType chosenType = m_engine->getPlayerType();

            m_engine->setStatBonuses(0, 0, 0);
            m_overworld->setPlayerCharacterType(chosenType);
            m_dungeon->setPlayerCharacterType(chosenType);

            if (m_currentSlot >= 0) {
                m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));
            }
        }

        updateGoldHud();

        m_overworld->activate();
        m_stack->setCurrentWidget(m_overworld);
        break;

    case GameState::Paused:
        break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Exploration
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onDungeonEntered()
{
    m_dungeon->setPlayerCharacterType(
        static_cast<CharacterType>(m_profile.characterType)
    );

    m_dungeon->setPlayerCharacter(m_engine->playerCharacter());
    m_dungeon->activate();
    m_stack->setCurrentWidget(m_dungeon);
}

void MainWindow::onExitedDungeon()
{
    m_profile.dungeonRuns++;

    Character* player = m_engine->playerCharacter();
    if (player) {
        player->resetHealth();
    }

    if (m_currentSlot >= 0) {
        m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));
    }

    updateGoldHud();

    m_overworld->activate();
    m_stack->setCurrentWidget(m_overworld);
}

void MainWindow::onDungeonGameOver(int coinsEarned, int wavesSurvived)
{
    m_gameOver->showDungeonResults(coinsEarned, wavesSurvived);
    m_stack->setCurrentWidget(m_gameOver);
}


void MainWindow::onBackToMenu()
{
    if (m_currentSlot >= 0) {
        m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));
    }

    m_playerHasChosen = false;
    m_hasPendingBattle = false;
    m_currentSlot = -1;

    m_profile.reset();
    m_engine->onExitToMenu();

    m_audio->playMusic("/music/menu.ogg");
    m_stack->setCurrentWidget(m_startScreen);
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

void MainWindow::onBuyItemRequested(ItemType type, int cost)
{
    if (m_profile.gold < cost) {
        return;
    }

    m_profile.spendGold(cost);
    m_profile.addItem(type, 1);

    if (m_currentSlot >= 0) {
        m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));
    }

    m_shop->setProfile(&m_profile);
    updateGoldHud();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Profile events
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onGoldEarned(int amount)
{
    m_profile.addGold(amount);
    updateGoldHud();
}

void MainWindow::onReturnToOverworld()
{
    m_profile.dungeonRuns++;

    Character* player = m_engine->playerCharacter();
    if (player) {
        player->resetHealth();
    }

    if (m_currentSlot >= 0) {
        m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));
    }

    updateGoldHud();

    m_overworld->activate();
    m_stack->setCurrentWidget(m_overworld);
}

void MainWindow::onSaveRequested()
{
    if (m_currentSlot < 0) {
        return;
    }

    m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));
}

void MainWindow::updateGoldHud()
{
    m_overworld->setGold(m_profile.gold);
    m_dungeon->setGold(m_profile.gold);
}

void MainWindow::onBattleItemChosen(ItemType type)
{
    Q_UNUSED(type);
}

void MainWindow::onLevelsEntered()
{
    m_levelSelect->refresh(m_profile);
    m_stack->setCurrentWidget(m_levelSelect);
}

void MainWindow::onLevelSelected(int levelId)
{
    const LevelDef* lvl = m_levelManager.level(levelId);
    if (!lvl) return;

    m_activeLevelId = levelId;
    m_inLevel       = true;

    // Show story slides first, then level starts in onStoryFinished
    m_storyDialog->setParent(this);
    m_storyDialog->resize(size());
    m_storyDialog->show(lvl->name, lvl->storyPages, lvl->enterPrompt);
    m_storyDialog->raise();
    m_storyDialog->setFocus();
}

void MainWindow::onStoryFinished()
{
    const LevelDef* lvl = m_levelManager.level(m_activeLevelId);
    if (!lvl) return;

    m_storyDialog->hide();

    m_level1->setPlayerCharacterType(
        static_cast<CharacterType>(m_profile.characterType));
    m_level1->activate(*lvl, m_profile);
    m_stack->setCurrentWidget(m_level1);
}


void MainWindow::onBossTriggered(const LevelDef& level)
{
    // Level ticker is already stopped inside Level1Widget
    m_bossDialog->setParent(m_level1);
    m_bossDialog->resize(m_level1->size());
    m_bossDialog->showIntro(level, m_profile.characterName);
    m_bossDialog->show();
    m_bossDialog->raise();
    m_bossDialog->setFocus();
}

void MainWindow::onBossFightAccepted()
{
    m_bossDialog->hide();
    m_level1->resumeAfterDialog();
}

void MainWindow::onBossOutroDismissed()
{
    m_bossDialog->hide();
    m_level1->deactivate();
    m_inLevel       = false;
    m_activeLevelId = 0;
    updateGoldHud();
    m_overworld->activate();
    m_stack->setCurrentWidget(m_overworld);
}

void MainWindow::onExitedLevel()
{
    // Player walked out of the exit portal after defeating the boss
    const LevelDef* lvl = m_levelManager.level(m_activeLevelId);
    bool alreadyCompleted = m_levelManager.isCompleted(m_activeLevelId, m_profile);

    if (lvl && !alreadyCompleted) {
        m_levelManager.completeLevel(m_activeLevelId, m_profile);
        if (m_currentSlot >= 0)
            m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));
        updateGoldHud();
    }

    // Show the boss outro dialog before going back to overworld
    if (lvl) {
        m_bossDialog->setParent(m_level1);
        m_bossDialog->resize(m_level1->size());
        m_bossDialog->showOutro(*lvl, true);   // true = player won
        m_bossDialog->show();
        m_bossDialog->raise();
        m_bossDialog->setFocus();
    } else {
        m_inLevel       = false;
        m_activeLevelId = 0;
        updateGoldHud();
        m_overworld->activate();
        m_stack->setCurrentWidget(m_overworld);
    }
}
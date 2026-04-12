#include "mainwindow.h"
#include "startscreenwidget.h"
#include "saveslotwidget.h"
#include "characterselectiondialog.h"
#include "battlewidget.h"
#include "gameoverwidget.h"
#include "scoreboardwidget.h"
#include "overworldwidget.h"
#include "dungeonwidget.h"
#include "audiomanager.h"
#include <QMenuBar>
#include <QAction>
#include <QApplication>
#include <QFile>

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
    m_battleWidget = new BattleWidget(m_engine, m_audio, this);
    m_gameOver     = new GameOverWidget(m_engine, this);
    m_scoreboard   = new ScoreboardWidget(m_engine, this);

    m_stack->addWidget(m_startScreen);
    m_stack->addWidget(m_slotScreen);
    m_stack->addWidget(m_charSelect);
    m_stack->addWidget(m_overworld);
    m_stack->addWidget(m_dungeon);
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
    connect(m_overworld, &OverworldWidget::backToMenu,
            this, &MainWindow::onBackToMenu);
    connect(m_overworld, &OverworldWidget::saveRequested,
            this, &MainWindow::onSaveRequested);

    // ── Dungeon ──────────────────────────────────────────────────────────────
    connect(m_dungeon, &DungeonWidget::battleTriggered,
            this, &MainWindow::onBattleTriggered);
    connect(m_dungeon, &DungeonWidget::exitedDungeon,
            this, &MainWindow::onExitedDungeon);
    connect(m_dungeon, &DungeonWidget::backToMenu,
            this, &MainWindow::onBackToMenu);

    // ── Game over ─────────────────────────────────────────────────────────────
    connect(m_gameOver, &GameOverWidget::returnToOverworld,
            this, &MainWindow::onReturnToOverworld);
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
                m_stack->setCurrentWidget(m_battleWidget);
                m_audio->playMusic("/music/battle.ogg");
            } else {
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
        m_stack->setCurrentWidget(m_gameOver);
        m_audio->stopMusic();
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

void MainWindow::onBattleTriggered(CharacterType enemyType, const QString& enemyName)
{
    m_pendingEnemyType = enemyType;
    m_pendingEnemyName = enemyName;

    if (!m_playerHasChosen) {
        // First encounter — need character select first
        m_hasPendingBattle = true;
        m_stack->setCurrentWidget(m_charSelect);
    } else {
        // Character already chosen — jump straight to battle
        m_engine->onPlayerSelectedCharacter(
            m_engine->getPlayerType(),
            m_engine->getPlayerName());
        // Engine emits PlayerTurn → onStateChanged shows battleWidget
    }
}

void MainWindow::onBackToMenu()
{
    m_playerHasChosen  = false;
    m_hasPendingBattle = false;
    m_currentSlot      = -1;
    m_profile.reset();
    m_engine->onRestartGame();   // emits stateChanged(MainMenu)
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
    if (m_currentSlot >= 0)
        m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));
    updateGoldHud();
    m_overworld->activate();
    m_stack->setCurrentWidget(m_overworld);
}

void MainWindow::updateGoldHud()
{
    m_overworld->setGold(m_profile.gold);
    m_dungeon->setGold(m_profile.gold);
}

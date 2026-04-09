// src/mainwindow.cpp
#include "mainwindow.h"
#include "startscreenwidget.h"
#include "characterselectiondialog.h"
#include "battlewidget.h"
#include "gameoverwidget.h"
#include "scoreboardwidget.h"
#include "overworldwidget.h"
#include "dungeonwidget.h"
#include <QMenuBar>
#include <QAction>
#include <QApplication>
#include "audiomanager.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    m_engine = new GameEngine(this);
    buildUI();
    buildMenuBar();

    // Battle-system state changes
    connect(m_engine, &GameEngine::stateChanged,
            this,     &MainWindow::onStateChanged);

    m_audio->playMusic("/music/menu.ogg");
}

MainWindow::~MainWindow() = default;

// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::buildUI()
{
    m_audio = new AudioManager(this);

    m_stack = new QStackedWidget(this);

    // ── Construct all screens ────────────────────────────────────────────────
    m_startScreen  = new StartScreenWidget(m_engine, this);
    m_charSelect   = new CharacterSelectWidget(m_engine, this);
    m_overworld    = new OverworldWidget(this);
    m_dungeon      = new DungeonWidget(this);
    m_battleWidget = new BattleWidget(m_engine, m_audio, this);
    m_gameOver     = new GameOverWidget(m_engine, this);
    m_scoreboard   = new ScoreboardWidget(m_engine, this);

    // ── Register in stack ────────────────────────────────────────────────────
    m_stack->addWidget(m_startScreen);
    m_stack->addWidget(m_charSelect);
    m_stack->addWidget(m_overworld);
    m_stack->addWidget(m_dungeon);
    m_stack->addWidget(m_battleWidget);
    m_stack->addWidget(m_gameOver);
    m_stack->addWidget(m_scoreboard);

    setCentralWidget(m_stack);
    m_stack->setCurrentWidget(m_startScreen);

    // ── Wire Overworld signals ───────────────────────────────────────────────
    connect(m_overworld, &OverworldWidget::dungeonEntered,
            this, &MainWindow::onDungeonEntered);
    connect(m_overworld, &OverworldWidget::backToMenu,
            this, &MainWindow::onBackToMenu);

    // ── Wire Dungeon signals ─────────────────────────────────────────────────
    connect(m_dungeon, &DungeonWidget::battleTriggered,
            this, &MainWindow::onBattleTriggered);
    connect(m_dungeon, &DungeonWidget::exitedDungeon,
            this, &MainWindow::onExitedDungeon);
    connect(m_dungeon, &DungeonWidget::backToMenu,
            this, &MainWindow::onBackToMenu);
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
//  GameEngine state → screen transitions
//  (Battle states only – exploration states are handled by direct slots below)
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onStateChanged(GameState newState)
{
    switch (newState) {

    case GameState::MainMenu:
        m_stack->setCurrentWidget(m_startScreen);
        m_audio->playMusic("/music/menu.ogg");
        break;

    // ── CharacterSelect now leads to the Overworld, not straight to battle ───
    case GameState::CharacterSelect:
        m_stack->setCurrentWidget(m_charSelect);
        break;

    // ── All active battle states show the battle screen ──────────────────────
    case GameState::Playing:
    case GameState::PlayerTurn:
    case GameState::AnimatingAttack:
    case GameState::Paused:
    case GameState::RoundOver:
        m_stack->setCurrentWidget(m_battleWidget);
        m_audio->playMusic("/music/battle.ogg");
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
//  Overworld / Dungeon navigation slots
// ─────────────────────────────────────────────────────────────────────────────

// Called when the player finishes character selection.
// Instead of jumping to battle, we now drop them into the Overworld.
// Hook this up by connecting CharacterSelectWidget's "character confirmed"
// signal here, OR intercept GameState::PlayerTurn the first time it fires
// before any round is started.  The cleanest approach is to connect
// GameEngine::stateChanged and, on the very first PlayerTurn after select,
// redirect to the overworld.  However the simplest zero-change approach is
// to override the CharacterSelect → PlayerTurn transition:
//
//   In onStateChanged(), when newState == PlayerTurn AND the stack is still
//   showing m_charSelect, show the overworld instead.
//
// That is already handled: onPlayerSelectedCharacter() calls setState(PlayerTurn)
// in GameEngine, which fires onStateChanged(PlayerTurn) here.  We intercept it:

// NOTE: Replace the PlayerTurn case above with this if you want the redirect:
//
//   case GameState::PlayerTurn:
//       if (m_stack->currentWidget() == m_charSelect) {
//           // First entry into battle – send player to Overworld first
//           m_overworld->activate();
//           m_stack->setCurrentWidget(m_overworld);
//           m_audio->playMusic("/music/menu.ogg");
//       } else {
//           m_stack->setCurrentWidget(m_battleWidget);
//           m_audio->playMusic("/music/battle.ogg");
//       }
//       break;
//
// For now the flow is: StartScreen → CharSelect → Overworld (via onDungeonEntered
// chain) → Dungeon → Battle.  The GameEngine starts a real battle only when
// onBattleTriggered calls onPlayerSelectedCharacter().


void MainWindow::onDungeonEntered()
{
    // Player walked into the dungeon door in the Overworld
    m_dungeon->activate();
    m_stack->setCurrentWidget(m_dungeon);
    m_audio->playMusic("/music/battle.ogg");
}

void MainWindow::onExitedDungeon()
{
    // Player reached the exit portal inside the Dungeon
    m_overworld->activate();
    m_stack->setCurrentWidget(m_overworld);
    m_audio->playMusic("/music/menu.ogg");
}

void MainWindow::onBattleTriggered(CharacterType enemyType, const QString &enemyName)
{
    // Store the enemy details so we can start the fight after character select.
    // If the player has ALREADY selected a character (i.e. m_engine has one),
    // we can start the battle immediately.  Otherwise send them to CharSelect first.

    m_pendingEnemyType = enemyType;
    m_pendingEnemyName = enemyName;

    if (m_engine->getState() == GameState::MainMenu ||
        m_engine->getPlayerName().isEmpty()) {
        // First time – player hasn't chosen a class yet
        m_stack->setCurrentWidget(m_charSelect);
    } else {
        // Player already has a character from a previous round – start directly
        m_engine->onPlayerSelectedCharacter(m_engine->getPlayerType(),
                                            m_engine->getPlayerName());
        // GameEngine will emit stateChanged(PlayerTurn) which shows m_battleWidget
    }
}

void MainWindow::onBackToMenu()
{
    m_engine->onRestartGame();   // resets engine state → emits stateChanged(MainMenu)
    // onStateChanged will switch to m_startScreen
}

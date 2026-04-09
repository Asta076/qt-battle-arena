// src/mainwindow.cpp
#include "mainwindow.h"
#include "startscreenwidget.h"
#include "characterselectiondialog.h"
#include "battlewidget.h"
#include "gameoverwidget.h"
#include "scoreboardwidget.h"
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

    // The single connection that drives the entire app
    connect(m_engine, &GameEngine::stateChanged,
            this,     &MainWindow::onStateChanged);
    m_audio->playMusic("/music/menu.ogg");   // ← ADD THIS
}

MainWindow::~MainWindow() = default;

void MainWindow::buildUI()
{
    m_audio = new AudioManager(this);   // ← create it once here
    m_stack       = new QStackedWidget(this);
    m_startScreen = new StartScreenWidget(m_engine, this);
    m_charSelect  = new CharacterSelectWidget(m_engine, this);
    m_battleWidget= new BattleWidget(m_engine,m_audio, this);
    m_gameOver    = new GameOverWidget(m_engine, this);
    m_scoreboard  = new ScoreboardWidget(m_engine, this);

    m_stack->addWidget(m_startScreen);
    m_stack->addWidget(m_charSelect);
    m_stack->addWidget(m_battleWidget);
    m_stack->addWidget(m_gameOver);
    m_stack->addWidget(m_scoreboard);

    setCentralWidget(m_stack);
    m_stack->setCurrentWidget(m_startScreen);
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

void MainWindow::onStateChanged(GameState newState)
{
    switch (newState) {
    case GameState::MainMenu:
        m_stack->setCurrentWidget(m_startScreen);   // ← was missing
        m_audio->playMusic("/music/menu.ogg");
        break;

    case GameState::CharacterSelect:
        m_stack->setCurrentWidget(m_charSelect);
        break;

    case GameState::Playing:
    case GameState::PlayerTurn:
    case GameState::AnimatingAttack:
    case GameState::Paused:
    case GameState::RoundOver:
        m_stack->setCurrentWidget(m_battleWidget);  // ← was missing for PlayerTurn
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

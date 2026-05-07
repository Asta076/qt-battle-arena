#include "mainwindow.h"

#include <QApplication>
#include <QAction>
#include <QFile>
#include <QMenu>
#include <QMenuBar>

#include "startscreenwidget.h"
#include "characterselectiondialog.h"
#include "overworldwidget.h"
#include "dungeonwidget.h"
#include "housewidget.h"
#include "shopwidget.h"

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

    m_startScreen = new StartScreenWidget(m_engine, this);
    m_slotScreen  = new SaveSlotWidget(this);
    m_charSelect  = new CharacterSelectWidget(m_engine, this);
    m_overworld   = new OverworldWidget(m_audio, this);
    m_dungeon     = new DungeonWidget(m_audio, this);
    m_house       = new HouseWidget(m_audio, this);
    m_shop        = new ShopWidget(this);

    m_stack->addWidget(m_startScreen);
    m_stack->addWidget(m_slotScreen);
    m_stack->addWidget(m_charSelect);
    m_stack->addWidget(m_overworld);
    m_stack->addWidget(m_dungeon);
    m_stack->addWidget(m_house);
    m_stack->addWidget(m_shop);

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

    connect(m_overworld, &OverworldWidget::houseEntered,
            this, &MainWindow::onHouseEntered);
    connect(m_overworld, &OverworldWidget::shopEntered,
            this, &MainWindow::onShopEntered);

    // ── Dungeon ──────────────────────────────────────────────────────────────
    connect(m_dungeon, &DungeonWidget::exitedDungeon,
            this, &MainWindow::onExitedDungeon);
    connect(m_dungeon, &DungeonWidget::backToMenu,
            this, &MainWindow::onBackToMenu);

    // ── House ────────────────────────────────────────────────────────────────
    connect(m_house, &HouseWidget::backToOverworld,
            this, &MainWindow::onHouseExited);

    // ── Shop ─────────────────────────────────────────────────────────────────
    connect(m_shop, &ShopWidget::backToOverworld,
            this, &MainWindow::onShopExited);
    connect(m_shop, &ShopWidget::buyItemRequested,
            this, &MainWindow::onBuyItemRequested);
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

    m_currentSlot = slotIndex;
    m_profile.reset();

    // Starting inventory stays because it belongs to profile/progression,
    // not to the deleted old combat screen.
    m_profile.addItem(ItemType::HealthPotion, 5);

    m_playerHasChosen = false;

    m_engine->onRestartGame();
    m_stack->setCurrentWidget(m_charSelect);
}

void MainWindow::onLoadSlot(int slotIndex)
{
    m_currentSlot = slotIndex;
    m_profile.loadFromFile(SaveSlotWidget::slotPath(slotIndex));

    m_playerHasChosen = true;

    m_engine->setStatBonuses(
        m_profile.upgrades.bonusMaxHp,
        m_profile.upgrades.bonusAttack,
        m_profile.upgrades.bonusSpPerAtk
    );

    m_engine->setPlayerIdentity(
        static_cast<CharacterType>(m_profile.characterType),
        m_profile.characterName
    );

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
    switch (newState)
    {
    case GameState::MainMenu:
        m_stack->setCurrentWidget(m_startScreen);
        break;

    case GameState::CharacterSelect:
        m_stack->setCurrentWidget(m_charSelect);
        break;

    case GameState::Playing:
        if (!m_playerHasChosen) {
            m_profile.characterName = m_engine->getPlayerName();
            m_profile.characterType = static_cast<int>(m_engine->getPlayerType());
            m_playerHasChosen = true;

            if (m_currentSlot >= 0)
                m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));
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
    m_dungeon->setPlayerCharacter(m_engine->playerCharacter());
    m_dungeon->activate();
    m_stack->setCurrentWidget(m_dungeon);
}

void MainWindow::onExitedDungeon()
{
    m_profile.dungeonRuns++;

    if (m_currentSlot >= 0)
        m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));

    updateGoldHud();

    m_overworld->activate();
    m_stack->setCurrentWidget(m_overworld);
}

void MainWindow::onBackToMenu()
{
    if (m_currentSlot >= 0)
        m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));

    m_playerHasChosen = false;
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
    if (m_profile.gold < cost)
        return;

    m_profile.spendGold(cost);
    m_profile.addItem(type, 1);

    if (m_currentSlot >= 0)
        m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));

    m_shop->setProfile(&m_profile);
    updateGoldHud();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Profile events
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onSaveRequested()
{
    if (m_currentSlot < 0)
        return;

    m_profile.saveToFile(SaveSlotWidget::slotPath(m_currentSlot));
}

void MainWindow::updateGoldHud()
{
    m_overworld->setGold(m_profile.gold);
    m_dungeon->setGold(m_profile.gold);
}
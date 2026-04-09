// include/mainwindow.h
#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include "gameengine.h"
#include "audiomanager.h"
// Forward declarations — no includes needed in the header
class StartScreenWidget;
class CharacterSelectWidget;
class BattleWidget;
class GameOverWidget;
class ScoreboardWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onStateChanged(GameState newState);

private:
    void buildUI();
    void buildMenuBar();
    GameEngine*            m_engine   = nullptr;
    QStackedWidget*        m_stack    = nullptr;

    StartScreenWidget*     m_startScreen    = nullptr;
    CharacterSelectWidget* m_charSelect     = nullptr;
    BattleWidget*          m_battleWidget   = nullptr;
    GameOverWidget*        m_gameOver       = nullptr;
    ScoreboardWidget*      m_scoreboard     = nullptr;
    AudioManager*          m_audio          = nullptr;
};

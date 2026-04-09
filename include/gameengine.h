#pragma once
#include <QObject>
#include <QTimer>
#include "character.h"
#include "battleresult.h"
#include "scoretracker.h"

enum class GameState {
    MainMenu,
    CharacterSelect,
    Playing,
    PlayerTurn,
    AnimatingAttack,
    Paused,
    RoundOver,
    GameOver,
    Scoreboard
};

enum class Difficulty { Easy, Normal, Hard };

class GameEngine : public QObject {
    Q_OBJECT

public:
    explicit GameEngine(QObject* parent = nullptr);
    ~GameEngine() override;

    // Read-only access for widgets
    GameState    getState()       const { return m_state; }
    Difficulty   getDifficulty()  const { return m_difficulty; }
    int          getPlayerScore() const { return m_playerScore; }
    int          getEnemyScore()  const { return m_enemyScore; }
    int          getCurrentRound()const { return m_currentRound; }
    int          getMaxRounds()   const { return m_maxRounds; }
    bool         itemAvailable()  const { return !m_itemUsed; }
    SessionStats getSessionStats()const { return m_tracker.getStats(); }

    // Needed by ScoreboardWidget to iterate all characters
    const QList<Character*>& getAllCharacters() const { return m_allCharacters; }


signals:
    void stateChanged(GameState newState);
    void healthUpdated(float playerPercent, float enemyPercent);
    void battleLogMessage(const QString& message);
    void battleActionResolved(BattleResult result);
    void roundEnded(int playerScore, int enemyScore, bool playerWonRound);
    void gameOver(bool playerWon, int finalPlayerScore, int finalEnemyScore);

public slots:
    // Called by StartScreenWidget
    void onStartGame();
    void onDifficultyChanged(Difficulty d);
    void setState(GameState s); // make public so GameOverWidget can call it


    // Called by CharacterSelectWidget
    void onPlayerSelectedCharacter(CharacterType type, const QString& name);

    // Called by BattleMenuWidget
    void onPlayerChoseFight();
    void onPlayerChoseSpecial();
    void onPlayerChoseItem();    // heals player; disabled after use
    void onPlayerChoseRun();     // forfeits the round

    // Called by MainWindow toolbar / PauseOverlay
    void onPauseToggle();
    void onRestartGame();
    void onExitToMenu();

    // Bonus: save / load
    bool onSaveGame(const QString& path);
    bool onLoadGame(const QString& path);

private slots:
    void enemyTakeTurn();        // fired by m_roundTimer after delay

private:
    void resolvePlayerAction(bool useSpecial);
    void spawnEnemy();           // randomly picks Warrior/Mage/Archer for CPU
    void endRound(bool playerWon);
    void cleanupCharacters();
    void resetRound();                  // ← ADD THIS
    Character* createCharacter(CharacterType type, const QString& name); // ← ADD THIS

    GameState    m_state         = GameState::MainMenu;
    Difficulty   m_difficulty    = Difficulty::Normal;

    Character*   m_player        = nullptr;
    Character*   m_enemy         = nullptr;
    QList<Character*> m_allCharacters; // full roster for scoreboard

    QTimer*      m_roundTimer    = nullptr; // delays enemy turn

    int          m_playerScore   = 0;
    int          m_enemyScore    = 0;
    int          m_currentRound  = 0;
    int          m_maxRounds     = 5;
    bool         m_itemUsed      = false;

    // AI pattern tracking (for Hard mode)
    QList<bool>  m_playerMoveHistory; // true = used special, false = basic attack

    ScoreTracker m_tracker;
    CharacterType m_playerType = CharacterType::Warrior;
    QString       m_playerName = "Player";
};
Q_DECLARE_METATYPE(Difficulty)

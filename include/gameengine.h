#pragma once

#include <QObject>
#include <QList>
#include <QString>

#include "character.h"
#include "scoretracker.h"

enum class GameState {
    MainMenu,
    CharacterSelect,
    Playing,
    Paused,
    GameOver,
    Scoreboard
};

enum class Difficulty {
    Easy,
    Normal,
    Hard
};

struct RoundRecord {
    int  roundNumber = 0;
    bool playerWon = false;
    QString enemyName;
};

class GameEngine : public QObject {
    Q_OBJECT

public:
    explicit GameEngine(QObject* parent = nullptr);
    ~GameEngine() override;

    GameState getState() const { return m_state; }
    Difficulty getDifficulty() const { return m_difficulty; }

    int getPlayerScore() const { return m_playerScore; }
    int getEnemyScore() const { return m_enemyScore; }
    int getCurrentRound() const { return m_currentRound; }
    int getMaxRounds() const { return m_maxRounds; }

    SessionStats getSessionStats() const { return m_tracker.getStats(); }

    QString getPlayerName() const { return m_playerName; }
    CharacterType getPlayerType() const { return m_playerType; }

    Character* playerCharacter() const { return m_player; }

    bool itemAvailable() const { return m_itemsUsedThisBattle < MAX_ITEMS_PER_BATTLE; }
    int itemsUsedThisBattle() const { return m_itemsUsedThisBattle; }
    int maxItemsPerBattle() const { return MAX_ITEMS_PER_BATTLE; }

    const QList<Character*>& getAllCharacters() const { return m_allCharacters; }
    const QList<RoundRecord>& getRoundHistory() const { return m_roundHistory; }

signals:
    void stateChanged(GameState newState);
    void gameOver(bool playerWon, int finalPlayerScore, int finalEnemyScore);
    void goldEarned(int amount);

public slots:
    void setState(GameState s);

    void onStartGame();
    void onDifficultyChanged(Difficulty d);
    void onPlayerSelectedCharacter(CharacterType type, const QString& name);

    void onPlayerHealed(int amount);
    void onPlayerSpRestored(int amount);
    void onPlayerAttackBoosted();
    void onPlayerDefenseActivated();

    void onPauseToggle();
    void onRestartGame();
    void onExitToMenu();

    bool onSaveGame(const QString& path);
    bool onLoadGame(const QString& path);

    void setPlayerIdentity(CharacterType type, const QString& name);
    void setStatBonuses(int bonusHp, int bonusAttack, int bonusSpPerAtk);

    void resetItemUses();
    void recordItemUse();

private:
    static constexpr int MAX_ITEMS_PER_BATTLE = 2;

    void cleanupCharacters();
    Character* createCharacter(CharacterType type, const QString& name);
    void applyPlayerBonuses();

    GameState m_state = GameState::MainMenu;
    Difficulty m_difficulty = Difficulty::Normal;

    Character* m_player = nullptr;
    QList<Character*> m_allCharacters;

    QList<RoundRecord> m_roundHistory;
    ScoreTracker m_tracker;

    int m_playerScore = 0;
    int m_enemyScore = 0;
    int m_currentRound = 0;
    int m_maxRounds = 5;

    int m_itemsUsedThisBattle = 0;

    bool m_defenseActive = false;
    bool m_attackBoosted = false;

    CharacterType m_playerType = CharacterType::Warrior;
    QString m_playerName = "Player";

    int m_bonusHp = 0;
    int m_bonusAttack = 0;
    int m_bonusSpPerAtk = 0;
};

Q_DECLARE_METATYPE(Difficulty)

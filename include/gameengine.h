#pragma once

#include <QObject>
#include <QString>

#include "character.h"

enum class GameState {
    MainMenu,
    CharacterSelect,
    Playing,
    Paused
};

enum class Difficulty {
    Easy,
    Normal,
    Hard
};

class GameEngine : public QObject {
    Q_OBJECT

public:
    explicit GameEngine(QObject* parent = nullptr);
    ~GameEngine() override;

    GameState getState() const { return m_state; }
    Difficulty getDifficulty() const { return m_difficulty; }

    QString getPlayerName() const { return m_playerName; }
    CharacterType getPlayerType() const { return m_playerType; }

    Character* playerCharacter() const { return m_player; }

    signals:
        void stateChanged(GameState newState);

public slots:
    void setState(GameState s);

    void onStartGame();
    void onDifficultyChanged(Difficulty d);
    void onPlayerSelectedCharacter(CharacterType type, const QString& name);

    void onPauseToggle();
    void onRestartGame();
    void onExitToMenu();

    bool onSaveGame(const QString& path);
    bool onLoadGame(const QString& path);

    void setPlayerIdentity(CharacterType type, const QString& name);
    void setStatBonuses(int bonusHp, int bonusAttack, int bonusSpPerAtk);

private:
    void cleanupCharacter();
    Character* createCharacter(CharacterType type, const QString& name);
    void applyPlayerBonuses();

    GameState m_state = GameState::MainMenu;
    Difficulty m_difficulty = Difficulty::Normal;

    Character* m_player = nullptr;

    CharacterType m_playerType = CharacterType::Warrior;
    QString m_playerName = "Player";

    int m_bonusHp = 0;
    int m_bonusAttack = 0;
    int m_bonusSpPerAtk = 0;
};

Q_DECLARE_METATYPE(Difficulty)
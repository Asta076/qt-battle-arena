#include "gameengine.h"
#include "warrior.h"
#include "mage.h"
#include "archer.h"
#include <QRandomGenerator>

GameEngine::GameEngine(QObject* parent)
    : QObject(parent)
{
    m_roundTimer = new QTimer(this);
    m_roundTimer->setSingleShot(true);
    connect(m_roundTimer, &QTimer::timeout, this, &GameEngine::enemyTakeTurn);
}

GameEngine::~GameEngine()
{
    cleanupCharacters();
}

// ── Helpers ────────────────────────────────────────────────────────────────

void GameEngine::setState(GameState s)
{
    m_state = s;
    emit stateChanged(s);
}

void GameEngine::cleanupCharacters()
{
    delete m_player; m_player = nullptr;
    delete m_enemy;  m_enemy  = nullptr;
    m_allCharacters.clear();
}

Character* GameEngine::createCharacter(CharacterType type, const QString& name)
{
    switch (type) {
    case CharacterType::Warrior: return new Warrior(name);
    case CharacterType::Mage:    return new Mage(name);
    case CharacterType::Archer:  return new Archer(name);
    }
    return new Warrior(name);
}

void GameEngine::spawnEnemy()
{
    const QStringList names = {
        "Shadow Warrior", "Dark Mage", "Black Arrow",
        "Iron Fist",      "Void Caster","Silent Hunter"
    };
    int typeRoll = QRandomGenerator::global()->bounded(3);
    int nameRoll = QRandomGenerator::global()->bounded(names.size());

    CharacterType type = static_cast<CharacterType>(typeRoll);
    delete m_enemy;
    m_enemy = createCharacter(type, names[nameRoll]);
}

void GameEngine::resetRound()
{
    // Recreate both characters at full health for the new round
    delete m_player;
    m_player = createCharacter(m_playerType, m_playerName);

    spawnEnemy();

    m_allCharacters.clear();
    m_allCharacters.append(m_player);
    m_allCharacters.append(m_enemy);

    m_itemUsed = false;
    emit healthUpdated(m_player->getHealthPercent(), m_enemy->getHealthPercent());
}

// ── Public Slots ───────────────────────────────────────────────────────────

void GameEngine::onStartGame()
{
    setState(GameState::CharacterSelect);
}

void GameEngine::onDifficultyChanged(Difficulty d)
{
    m_difficulty = d;
}

void GameEngine::onPlayerSelectedCharacter(CharacterType type, const QString& name)
{
    m_playerType = type;
    m_playerName = name;

    cleanupCharacters();

    m_player = createCharacter(type, name);
    spawnEnemy();

    m_allCharacters.append(m_player);
    m_allCharacters.append(m_enemy);

    m_playerScore  = 0;
    m_enemyScore   = 0;
    m_currentRound = 1;
    m_itemUsed     = false;
    m_playerMoveHistory.clear();
    m_tracker.reset();

    emit healthUpdated(m_player->getHealthPercent(), m_enemy->getHealthPercent());
    emit battleLogMessage(QString("Round %1 — %2 vs %3!")
                              .arg(m_currentRound)
                              .arg(m_player->getName())
                              .arg(m_enemy->getName()));

    setState(GameState::PlayerTurn);
}

void GameEngine::resolvePlayerAction(bool useSpecial)
{
    if (m_state != GameState::PlayerTurn) return;
    setState(GameState::AnimatingAttack);

    int damage = useSpecial ? m_player->specialAbility() : m_player->attack();
    m_playerMoveHistory.append(useSpecial);
    if (m_playerMoveHistory.size() > 5) m_playerMoveHistory.removeFirst();

    m_enemy->takeDamage(damage);

    BattleResult result;
    result.attackerName    = m_player->getName();
    result.defenderName    = m_enemy->getName();
    result.damageDone      = damage;
    result.usedSpecial     = useSpecial;
    result.defenderFainted = !m_enemy->isAlive();

    emit battleActionResolved(result);
    emit healthUpdated(m_player->getHealthPercent(), m_enemy->getHealthPercent());
    emit battleLogMessage(
        useSpecial
            ? QString("%1 unleashes Special for %2 dmg!").arg(m_player->getName()).arg(damage)
            : QString("%1 attacks for %2 dmg!").arg(m_player->getName()).arg(damage)
        );

    if (!m_enemy->isAlive()) {
        emit battleLogMessage(m_enemy->getName() + " fainted!");
        QTimer::singleShot(1400, this, [this]{ endRound(true); });
    } else {
        // Delay based on difficulty
        int delay = (m_difficulty == Difficulty::Hard) ? 550
                    : (m_difficulty == Difficulty::Easy) ? 1300 : 900;
        m_roundTimer->start(delay);
    }
}

void GameEngine::onPlayerChoseFight()   { resolvePlayerAction(false); }
void GameEngine::onPlayerChoseSpecial() { resolvePlayerAction(true);  }

void GameEngine::onPlayerChoseItem()
{
    if (m_itemUsed || m_state != GameState::PlayerTurn) return;

    setState(GameState::AnimatingAttack);
    m_itemUsed = true;

    int healAmount = static_cast<int>(m_player->getMaxHealth() * 0.35f);
    m_player->heal(healAmount);

    emit healthUpdated(m_player->getHealthPercent(), m_enemy->getHealthPercent());
    emit battleLogMessage(
        QString("%1 used a Potion! Recovered %2 HP!")
            .arg(m_player->getName()).arg(healAmount)
        );

    // Enemy still gets its turn
    m_roundTimer->start(900);
}

void GameEngine::onPlayerChoseRun()
{
    if (m_state != GameState::PlayerTurn) return;
    m_roundTimer->stop();
    emit battleLogMessage(m_player->getName() + " fled! Round forfeited.");
    endRound(false);
}

// ── Enemy AI ───────────────────────────────────────────────────────────────

void GameEngine::enemyTakeTurn()
{
    if (!m_enemy->isAlive() || !m_player->isAlive()) return;

    bool useSpecial = false;
    int  damage     = 0;

    switch (m_difficulty) {
    case Difficulty::Easy:
        damage = m_enemy->attack();
        break;

    case Difficulty::Normal:
        useSpecial = QRandomGenerator::global()->bounded(100) < 25;
        damage = useSpecial ? m_enemy->specialAbility() : m_enemy->attack();
        break;

    case Difficulty::Hard: {
        // Use special if enemy is below 40% health
        bool lowHealth = m_enemy->getHealthPercent() < 0.4f;

        // Detect if player keeps spamming basic attack (last 3 moves all basic)
        bool playerPredictable = false;
        if (m_playerMoveHistory.size() >= 3) {
            playerPredictable = !m_playerMoveHistory[m_playerMoveHistory.size()-1]
                                && !m_playerMoveHistory[m_playerMoveHistory.size()-2]
                                && !m_playerMoveHistory[m_playerMoveHistory.size()-3];
        }

        useSpecial = lowHealth || playerPredictable;
        damage = useSpecial ? m_enemy->specialAbility() : m_enemy->attack();
        break;
    }
    }

    m_player->takeDamage(damage);

    BattleResult result;
    result.attackerName    = m_enemy->getName();
    result.defenderName    = m_player->getName();
    result.damageDone      = damage;
    result.usedSpecial     = useSpecial;
    result.defenderFainted = !m_player->isAlive();

    emit battleActionResolved(result);
    emit healthUpdated(m_player->getHealthPercent(), m_enemy->getHealthPercent());
    emit battleLogMessage(
        useSpecial
            ? QString("%1 unleashes Special for %2 dmg!").arg(m_enemy->getName()).arg(damage)
            : QString("%1 attacks for %2 dmg!").arg(m_enemy->getName()).arg(damage)
        );

    if (!m_player->isAlive()) {
        emit battleLogMessage(m_player->getName() + " fainted!");
        QTimer::singleShot(1400, this, [this]{ endRound(false); });
    } else {
        setState(GameState::PlayerTurn);
    }
}

// ── Round + Game flow ──────────────────────────────────────────────────────

void GameEngine::endRound(bool playerWon)
{
    m_roundTimer->stop();

    if (playerWon) { ++m_playerScore; m_tracker.recordWin();  }
    else           { ++m_enemyScore;  m_tracker.recordLoss(); }

    emit roundEnded(m_playerScore, m_enemyScore, playerWon);
    setState(GameState::RoundOver);

    // First to 3 wins, or whoever leads after 5 rounds
    bool isGameOver = (m_playerScore >= 3)
                      || (m_enemyScore  >= 3)
                      || (m_currentRound >= m_maxRounds);


    if (isGameOver) {
        QTimer::singleShot(2200, this, [this]{
            setState(GameState::GameOver);
            emit gameOver(m_playerScore > m_enemyScore,
                          m_playerScore, m_enemyScore);
        });
    } else {
        ++m_currentRound;
        QTimer::singleShot(2500, this, [this]{
            resetRound();
            emit battleLogMessage(
                QString("Round %1 — Fight!").arg(m_currentRound)
                );
            setState(GameState::PlayerTurn);
        });
    }
}

void GameEngine::onPauseToggle()
{
    if (m_state == GameState::PlayerTurn || m_state == GameState::Playing) {
        m_roundTimer->stop();
        setState(GameState::Paused);
    } else if (m_state == GameState::Paused) {
        setState(GameState::PlayerTurn);
    }
}

void GameEngine::onRestartGame()
{
    m_roundTimer->stop();
    cleanupCharacters();
    m_playerScore  = 0;
    m_enemyScore   = 0;
    m_currentRound = 1;
    m_itemUsed     = false;
    m_playerMoveHistory.clear();
    m_tracker.reset();
    setState(GameState::MainMenu);
}

void GameEngine::onExitToMenu() { onRestartGame(); }

bool GameEngine::onSaveGame(const QString& path) { return m_tracker.saveToFile(path); }
bool GameEngine::onLoadGame(const QString& path) { return m_tracker.loadFromFile(path); }

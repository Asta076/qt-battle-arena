#include "gameengine.h"

#include "warrior.h"
#include "mage.h"
#include "archer.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

GameEngine::GameEngine(QObject* parent)
    : QObject(parent)
{
}

GameEngine::~GameEngine()
{
    cleanupCharacters();
}

void GameEngine::setState(GameState s)
{
    if (m_state == s)
        return;

    m_state = s;
    emit stateChanged(s);
}

void GameEngine::cleanupCharacters()
{
    delete m_player;
    m_player = nullptr;
    m_allCharacters.clear();
}

Character* GameEngine::createCharacter(CharacterType type, const QString& name)
{
    switch (type) {
    case CharacterType::Warrior:
        return new Warrior(name);
    case CharacterType::Mage:
        return new Mage(name);
    case CharacterType::Archer:
        return new Archer(name);
    }

    return new Warrior(name);
}

void GameEngine::applyPlayerBonuses()
{
    if (!m_player)
        return;

    m_player->applyBonusHealth(m_bonusHp);
    m_player->applyBonusAttack(m_bonusAttack);
    m_player->applyBonusSpPerAtk(m_bonusSpPerAtk);
}

void GameEngine::onStartGame()
{
    cleanupCharacters();

    m_playerScore = 0;
    m_enemyScore = 0;
    m_currentRound = 0;
    m_itemsUsedThisBattle = 0;
    m_roundHistory.clear();
    m_tracker.reset();

    setState(GameState::CharacterSelect);
}

void GameEngine::onDifficultyChanged(Difficulty d)
{
    m_difficulty = d;
}

void GameEngine::onPlayerSelectedCharacter(CharacterType type, const QString& name)
{
    cleanupCharacters();

    m_playerType = type;
    m_playerName = name.trimmed().isEmpty() ? "Player" : name.trimmed();

    m_player = createCharacter(m_playerType, m_playerName);
    applyPlayerBonuses();

    if (m_player)
        m_allCharacters.append(m_player);

    setState(GameState::Playing);
}

void GameEngine::setPlayerIdentity(CharacterType type, const QString& name)
{
    m_playerType = type;
    m_playerName = name.trimmed().isEmpty() ? "Player" : name.trimmed();

    if (m_player) {
        cleanupCharacters();
        m_player = createCharacter(m_playerType, m_playerName);
        applyPlayerBonuses();
        m_allCharacters.append(m_player);
    }
}

void GameEngine::setStatBonuses(int bonusHp, int bonusAttack, int bonusSpPerAtk)
{
    m_bonusHp = bonusHp;
    m_bonusAttack = bonusAttack;
    m_bonusSpPerAtk = bonusSpPerAtk;

    if (m_player) {
        CharacterType type = m_playerType;
        QString name = m_playerName;

        cleanupCharacters();

        m_player = createCharacter(type, name);
        applyPlayerBonuses();
        m_allCharacters.append(m_player);
    }
}

void GameEngine::onPlayerHealed(int amount)
{
    if (!m_player || amount <= 0)
        return;

    m_player->heal(amount);
    recordItemUse();
}

void GameEngine::onPlayerSpRestored(int amount)
{
    if (!m_player || amount <= 0)
        return;

    m_player->addSp(amount);
    recordItemUse();
}

void GameEngine::onPlayerAttackBoosted()
{
    if (!m_player)
        return;

    if (!m_attackBoosted) {
        m_player->applyBonusAttack(10);
        m_attackBoosted = true;
    }

    recordItemUse();
}

void GameEngine::onPlayerDefenseActivated()
{
    m_defenseActive = true;
    recordItemUse();
}

void GameEngine::resetItemUses()
{
    m_itemsUsedThisBattle = 0;
    m_attackBoosted = false;
    m_defenseActive = false;
}

void GameEngine::recordItemUse()
{
    if (m_itemsUsedThisBattle < MAX_ITEMS_PER_BATTLE)
        ++m_itemsUsedThisBattle;
}

void GameEngine::onPauseToggle()
{
    if (m_state == GameState::Paused) {
        setState(GameState::Playing);
    } else if (m_state == GameState::Playing) {
        setState(GameState::Paused);
    }
}

void GameEngine::onRestartGame()
{
    cleanupCharacters();

    m_playerScore = 0;
    m_enemyScore = 0;
    m_currentRound = 0;
    m_itemsUsedThisBattle = 0;
    m_roundHistory.clear();
    m_tracker.reset();

    setState(GameState::CharacterSelect);
}

void GameEngine::onExitToMenu()
{
    cleanupCharacters();

    m_playerScore = 0;
    m_enemyScore = 0;
    m_currentRound = 0;
    m_itemsUsedThisBattle = 0;
    m_roundHistory.clear();

    setState(GameState::MainMenu);
}

bool GameEngine::onSaveGame(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QJsonObject root;

    root["playerName"] = m_playerName;
    root["playerType"] = static_cast<int>(m_playerType);
    root["difficulty"] = static_cast<int>(m_difficulty);

    root["playerScore"] = m_playerScore;
    root["enemyScore"] = m_enemyScore;
    root["currentRound"] = m_currentRound;
    root["maxRounds"] = m_maxRounds;

    root["bonusHp"] = m_bonusHp;
    root["bonusAttack"] = m_bonusAttack;
    root["bonusSpPerAtk"] = m_bonusSpPerAtk;

    QJsonDocument doc(root);
    file.write(doc.toJson());

    return true;
}

bool GameEngine::onLoadGame(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    const QByteArray data = file.readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject())
        return false;

    const QJsonObject root = doc.object();

    m_playerName = root.value("playerName").toString("Player");
    m_playerType = static_cast<CharacterType>(root.value("playerType").toInt(0));
    m_difficulty = static_cast<Difficulty>(root.value("difficulty").toInt(1));

    m_playerScore = root.value("playerScore").toInt(0);
    m_enemyScore = root.value("enemyScore").toInt(0);
    m_currentRound = root.value("currentRound").toInt(0);
    m_maxRounds = root.value("maxRounds").toInt(5);

    m_bonusHp = root.value("bonusHp").toInt(0);
    m_bonusAttack = root.value("bonusAttack").toInt(0);
    m_bonusSpPerAtk = root.value("bonusSpPerAtk").toInt(0);

    cleanupCharacters();

    m_player = createCharacter(m_playerType, m_playerName);
    applyPlayerBonuses();

    if (m_player)
        m_allCharacters.append(m_player);

    setState(GameState::Playing);

    return true;
}

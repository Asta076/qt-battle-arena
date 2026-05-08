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
    cleanupCharacter();
}

void GameEngine::setState(GameState s)
{
    if (m_state == s)
        return;

    m_state = s;
    emit stateChanged(s);
}

void GameEngine::cleanupCharacter()
{
    delete m_player;
    m_player = nullptr;
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
    cleanupCharacter();
    setState(GameState::CharacterSelect);
}

void GameEngine::onDifficultyChanged(Difficulty d)
{
    m_difficulty = d;
}

void GameEngine::onPlayerSelectedCharacter(CharacterType type, const QString& name)
{
    cleanupCharacter();

    m_playerType = type;
    m_playerName = name.trimmed().isEmpty() ? "Player" : name.trimmed();

    m_player = createCharacter(m_playerType, m_playerName);
    applyPlayerBonuses();

    setState(GameState::Playing);
}

void GameEngine::setPlayerIdentity(CharacterType type, const QString& name)
{
    cleanupCharacter();

    m_playerType = type;
    m_playerName = name.trimmed().isEmpty() ? "Player" : name.trimmed();

    m_player = createCharacter(m_playerType, m_playerName);
    applyPlayerBonuses();
}

void GameEngine::setStatBonuses(int bonusHp, int bonusAttack, int bonusSpPerAtk)
{
    m_bonusHp = bonusHp;
    m_bonusAttack = bonusAttack;
    m_bonusSpPerAtk = bonusSpPerAtk;

    if (m_player) {
        CharacterType type = m_playerType;
        QString name = m_playerName;

        cleanupCharacter();

        m_player = createCharacter(type, name);
        applyPlayerBonuses();
    }
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
    cleanupCharacter();
    setState(GameState::CharacterSelect);
}

void GameEngine::onExitToMenu()
{
    cleanupCharacter();
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

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return false;

    QJsonObject root = doc.object();

    m_playerName = root.value("playerName").toString("Player");
    m_playerType = static_cast<CharacterType>(root.value("playerType").toInt(0));
    m_difficulty = static_cast<Difficulty>(root.value("difficulty").toInt(1));

    m_bonusHp = root.value("bonusHp").toInt(0);
    m_bonusAttack = root.value("bonusAttack").toInt(0);
    m_bonusSpPerAtk = root.value("bonusSpPerAtk").toInt(0);

    cleanupCharacter();

    m_player = createCharacter(m_playerType, m_playerName);
    applyPlayerBonuses();

    setState(GameState::Playing);
    return true;
}
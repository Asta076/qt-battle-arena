#pragma once

#include <QList>
#include <QPointF>
#include <QRectF>
#include <QString>

#include "character.h"

class EnemySprite;

struct DungeonEnemySpawn
{
    CharacterType type = CharacterType::Warrior;
    QString name;
    int hp = 0;
    int damage = 0;
    QPointF position;
};

class DungeonRunManager
{
public:
    void reset();

    int waveNumber() const { return m_waveNumber; }
    int coinsEarned() const { return m_coinsEarned; }
    int wavesSurvived() const;

    QList<DungeonEnemySpawn> createWaveSpawns() const;
    void advanceWave();

    int registerKillReward();
    int healAmountForKill(const Character& player) const;

    void updateEnemyMovement(const QList<EnemySprite*>& enemies,
                             const QRectF& playerBounds,
                             const QRectF& worldBounds) const;

    static constexpr int GOLD_PER_KILL = 3;
    static constexpr float SPECIAL_PER_KILL = 0.10f;

private:
    int m_waveNumber = 1;
    int m_coinsEarned = 0;

    static constexpr int MAX_ENEMIES_PER_WAVE = 10;

    void separateEnemies(const QList<EnemySprite*>& enemies,
                         const QRectF& worldBounds) const;
};

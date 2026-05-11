#include "dungeonrunmanager.h"

#include <algorithm>
#include <cmath>

#include <QRandomGenerator>

#include "dungeonwidget.h"

void DungeonRunManager::reset()
{
    m_waveNumber = 1;
    m_coinsEarned = 0;
}

int DungeonRunManager::wavesSurvived() const
{
    return std::max(0, m_waveNumber - 1);
}

QList<DungeonEnemySpawn> DungeonRunManager::createWaveSpawns() const
{
    const int enemyCount = std::min(MAX_ENEMIES_PER_WAVE, 3 + (m_waveNumber - 1) * 2);
    const int hpBonus = (m_waveNumber - 1) * 10;
    const int damageBonus = (m_waveNumber - 1) * 2;

    const QList<QPointF> spawnPoints = {
        QPointF(120, 120),
        QPointF(630, 120),
        QPointF(120, 430),
        QPointF(630, 430),
        QPointF(400, 120),
        QPointF(400, 430),
        QPointF(180, 260),
        QPointF(590, 260)
    };

    QList<DungeonEnemySpawn> spawns;
    spawns.reserve(enemyCount);

    for (int i = 0; i < enemyCount; ++i) {
        DungeonEnemySpawn spawn;

        switch (i % 3) {
        case 0:
            spawn.type = CharacterType::Warrior;
            spawn.name = "Skeleton Guard";
            spawn.hp = 75 + hpBonus;
            spawn.damage = 10 + damageBonus;
            break;
        case 1:
            spawn.type = CharacterType::Mage;
            spawn.name = "Goblin Brute";
            spawn.hp = 50 + hpBonus;
            spawn.damage = 14 + damageBonus;
            break;
        default:
            spawn.type = CharacterType::Archer;
            spawn.name = "Goblin";
            spawn.hp = 55 + hpBonus;
            spawn.damage = 8 + damageBonus;
            break;
        }

        spawn.position = spawnPoints[i % spawnPoints.size()];

        if (i >= spawnPoints.size()) {
            spawn.position += QPointF(
                QRandomGenerator::global()->bounded(-40, 41),
                QRandomGenerator::global()->bounded(-40, 41)
            );
        }

        spawns.append(spawn);
    }

    return spawns;
}

void DungeonRunManager::advanceWave()
{
    ++m_waveNumber;
}

int DungeonRunManager::registerKillReward()
{
    m_coinsEarned += GOLD_PER_KILL;
    return GOLD_PER_KILL;
}

int DungeonRunManager::healAmountForKill(const Character& player) const
{
    return std::max(1, static_cast<int>(player.getMaxHealth() * 0.10f));
}

void DungeonRunManager::updateEnemyMovement(const QList<EnemySprite*>& enemies,
                                            const QRectF& playerBounds,
                                            const QRectF& worldBounds) const
{
    for (EnemySprite* enemy : enemies) {
        if (!enemy)
            continue;

        enemy->chasePlayer(playerBounds, worldBounds);
    }

    separateEnemies(enemies, worldBounds);

    for (EnemySprite* enemy : enemies) {
        if (!enemy)
            continue;

        enemy->updateAnimation();
    }
}

void DungeonRunManager::separateEnemies(const QList<EnemySprite*>& enemies,
                                        const QRectF& worldBounds) const
{
    for (int i = 0; i < enemies.size(); ++i) {
        EnemySprite* a = enemies[i];
        if (!a)
            continue;

        for (int j = i + 1; j < enemies.size(); ++j) {
            EnemySprite* b = enemies[j];
            if (!b)
                continue;

            QRectF aBox = a->hitBox();
            QRectF bBox = b->hitBox();

            if (!aBox.intersects(bBox))
                continue;

            QPointF delta = aBox.center() - bBox.center();
            qreal length = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());

            if (length < 0.001) {
                delta = QPointF(1.0, 0.0);
                length = 1.0;
            }

            QPointF pushDir(delta.x() / length, delta.y() / length);
            QRectF overlap = aBox.intersected(bBox);
            qreal pushAmount = std::min(overlap.width(), overlap.height()) / 2.0 + 0.5;
            QPointF push = pushDir * pushAmount;

            QPointF aNext = a->pos() + push;
            QPointF bNext = b->pos() - push;

            aNext.setX(std::clamp(aNext.x(), worldBounds.left(), worldBounds.right() - EnemySprite::W));
            aNext.setY(std::clamp(aNext.y(), worldBounds.top(), worldBounds.bottom() - EnemySprite::H));
            bNext.setX(std::clamp(bNext.x(), worldBounds.left(), worldBounds.right() - EnemySprite::W));
            bNext.setY(std::clamp(bNext.y(), worldBounds.top(), worldBounds.bottom() - EnemySprite::H));

            a->setPos(aNext);
            b->setPos(bNext);
        }
    }
}

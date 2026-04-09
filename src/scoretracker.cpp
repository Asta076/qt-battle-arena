#include "scoretracker.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

void ScoreTracker::recordWin()   { ++m_stats.playerWins;   ++m_stats.roundsPlayed; }
void ScoreTracker::recordLoss()  { ++m_stats.playerLosses; ++m_stats.roundsPlayed; }
void ScoreTracker::recordDraw()  { ++m_stats.draws;        ++m_stats.roundsPlayed; }

SessionStats ScoreTracker::getStats() const { return m_stats; }

void ScoreTracker::reset() { m_stats = SessionStats{}; }

bool ScoreTracker::saveToFile(const QString& path) const
{
    QJsonObject obj;
    obj["wins"]         = m_stats.playerWins;
    obj["losses"]       = m_stats.playerLosses;
    obj["draws"]        = m_stats.draws;
    obj["roundsPlayed"] = m_stats.roundsPlayed;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(obj).toJson());
    return true;
}

bool ScoreTracker::loadFromFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
    m_stats.playerWins   = obj["wins"].toInt();
    m_stats.playerLosses = obj["losses"].toInt();
    m_stats.draws        = obj["draws"].toInt();
    m_stats.roundsPlayed = obj["roundsPlayed"].toInt();
    return true;
}

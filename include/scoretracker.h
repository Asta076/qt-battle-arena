#pragma once
#include <QString>

struct SessionStats {
    int playerWins   = 0;
    int playerLosses = 0;
    int draws        = 0;
    int roundsPlayed = 0;
};

class ScoreTracker {
public:
    ScoreTracker() = default;

    void recordWin();
    void recordLoss();
    void recordDraw();

    SessionStats getStats() const;
    void reset();

    // Bonus: file persistence
    bool saveToFile(const QString& path) const;
    bool loadFromFile(const QString& path);

private:
    SessionStats m_stats;
};

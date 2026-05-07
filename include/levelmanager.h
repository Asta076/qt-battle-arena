#pragma once
#include <QList>
#include "leveldef.h"
#include "playerprofile.h"

// ─────────────────────────────────────────────────────────────────────────────
//  LevelManager — owns all level definitions, checks unlock/completion state
//  Owned by MainWindow. Read-only access passed to OverworldWidget for drawing.
// ─────────────────────────────────────────────────────────────────────────────
class LevelManager {
public:
    LevelManager();

    const QList<LevelDef>& levels()          const { return m_levels; }
    const LevelDef*        level(int id)     const;  // nullptr if not found

    bool isUnlocked (int levelId, const PlayerProfile& p) const;
    bool isCompleted(int levelId, const PlayerProfile& p) const;

    // Call after beating a boss — marks level complete, returns gold reward
    int  completeLevel(int levelId, PlayerProfile& p) const;

private:
    void buildLevels();
    QList<LevelDef> m_levels;
};
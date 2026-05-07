#include "levelmanager.h"

LevelManager::LevelManager()
{
    buildLevels();
}

void LevelManager::buildLevels()
{
    // ── Level 1 — always open, gentle intro ──────────────────────────────────
    m_levels.append({
        .id               = 1,
        .name             = "Goblin Cave",
        .bossName         = "Grunt Chief",
        .bossType         = CharacterType::Warrior,
        .bossHpMultiplier = 2,
        .bossAttackBonus  = 5,
        .bossGoldReward   = 150,
        .requiredRuns     = 0,
        .requiredGold     = 0,
        .requiredLevelId  = 0,
        .bossIntroLine    = "So... another fool wanders into my cave.",
        .bossDefeatLine   = "Impossible! Defeated by a newcomer!",
        .bossVictoryLine  = "Hah! Run back to the surface, weakling.",
        .unlockHint       = ""
    });

    // ── Level 2 — needs L1 done + 3 runs ─────────────────────────────────────
    m_levels.append({
        .id               = 2,
        .name             = "Haunted Forest",
        .bossName         = "Shadow Walker",
        .bossType         = CharacterType::Archer,
        .bossHpMultiplier = 2,
        .bossAttackBonus  = 8,
        .bossGoldReward   = 250,
        .requiredRuns     = 3,
        .requiredGold     = 0,
        .requiredLevelId  = 1,
        .bossIntroLine    = "You smell of the cave. How disappointing.",
        .bossDefeatLine   = "The shadows... they fail me.",
        .bossVictoryLine  = "Leave this forest. You are not ready.",
        .unlockHint       = "Beat Level 1 + 3 dungeon runs"
    });

    // ── Level 3 — needs L2 done + 6 runs + 200G ──────────────────────────────
    m_levels.append({
        .id               = 3,
        .name             = "Frozen Peak",
        .bossName         = "Frost Mage",
        .bossType         = CharacterType::Mage,
        .bossHpMultiplier = 3,
        .bossAttackBonus  = 10,
        .bossGoldReward   = 400,
        .requiredRuns     = 6,
        .requiredGold     = 200,
        .requiredLevelId  = 2,
        .bossIntroLine    = "You climbed all this way? Impressive... and futile.",
        .bossDefeatLine   = "The cold... it was not enough.",
        .bossVictoryLine  = "Your warmth fades here. Go.",
        .unlockHint       = "Beat Level 2 + 6 runs + 200 Gold"
    });

    // ── Level 4 — needs L3 done + 10 runs ────────────────────────────────────
    m_levels.append({
        .id               = 4,
        .name             = "Volcanic Dungeon",
        .bossName         = "Ember Warrior",
        .bossType         = CharacterType::Warrior,
        .bossHpMultiplier = 3,
        .bossAttackBonus  = 15,
        .bossGoldReward   = 600,
        .requiredRuns     = 10,
        .requiredGold     = 0,
        .requiredLevelId  = 3,
        .bossIntroLine    = "The volcano chose me. What did you bring?",
        .bossDefeatLine   = "The fire dies... as do I.",
        .bossVictoryLine  = "You will be ash before the summit.",
        .unlockHint       = "Beat Level 3 + 10 dungeon runs"
    });

    // ── Level 5 — the finale, beat all 4 ─────────────────────────────────────
    m_levels.append({
        .id               = 5,
        .name             = "The Final Arena",
        .bossName         = "The Champion",
        .bossType         = CharacterType::Warrior,  // overridden to player's type at runtime
        .bossHpMultiplier = 4,
        .bossAttackBonus  = 20,
        .bossGoldReward   = 1000,
        .requiredRuns     = 0,
        .requiredGold     = 0,
        .requiredLevelId  = 4,
        .bossIntroLine    = "I am everything you could have been. Prove me wrong.",
        .bossDefeatLine   = "...You have surpassed me. Well fought.",
        .bossVictoryLine  = "You were not ready. Come back stronger.",
        .unlockHint       = "Beat all 4 levels"
    });
}

const LevelDef* LevelManager::level(int id) const
{
    for (const LevelDef& l : m_levels)
        if (l.id == id) return &l;
    return nullptr;
}

bool LevelManager::isUnlocked(int levelId, const PlayerProfile& p) const
{
    const LevelDef* l = level(levelId);
    if (!l) return false;
    if (l->requiredRuns     > 0 && p.dungeonRuns    < l->requiredRuns)     return false;
    if (l->requiredGold     > 0 && p.gold           < l->requiredGold)     return false;
    if (l->requiredLevelId  > 0 && !p.completedLevels.contains(l->requiredLevelId)) return false;
    return true;
}

bool LevelManager::isCompleted(int levelId, const PlayerProfile& p) const
{
    return p.completedLevels.contains(levelId);
}

int LevelManager::completeLevel(int levelId, PlayerProfile& p) const
{
    const LevelDef* l = level(levelId);
    if (!l) return 0;
    p.completedLevels.insert(levelId);
    p.addGold(l->bossGoldReward);
    return l->bossGoldReward;
}
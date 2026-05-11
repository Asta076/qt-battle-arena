#pragma once
#include <QString>
#include <QStringList>
#include "character.h"

// ─────────────────────────────────────────────────────────────────────────────
//  LevelTheme — controls the visual mood of the level scene
//  Used by Level1Widget::buildScene() to apply palette and atmosphere
// ─────────────────────────────────────────────────────────────────────────────
enum class LevelTheme {
    Cave,      // Level 1 — green grass, dirt path, normal lighting
    Forest,    // Level 2 — dark purple overlay, haunted mood
    Peak,      // Level 3 — blue/grey icy overlay
    Volcano,   // Level 4 — red/orange fiery overlay
    Arena      // Level 5 — dark stone, minimal color
};

// ─────────────────────────────────────────────────────────────────────────────
//  LevelDef — pure data, no Qt beyond QString/QStringList
//  Add a new level by adding a row to LevelManager::buildLevels()
// ─────────────────────────────────────────────────────────────────────────────
struct LevelDef {
    int           id;
    QString       name;
    LevelTheme    theme = LevelTheme::Cave;  // ← controls visual mood

    // ── Boss identity ─────────────────────────────────────────────────────────
    QString       bossName;
    CharacterType bossType;
    int           bossHpMultiplier;
    int           bossAttackBonus;
    int           bossGoldReward;

    // ── Unlock criteria ───────────────────────────────────────────────────────
    int           requiredRuns;
    int           requiredGold;
    int           requiredLevelId;

    // ── Boss dialogue ─────────────────────────────────────────────────────────
    QString       bossIntroLine;
    QString       bossDefeatLine;
    QString       bossVictoryLine;
    QString       unlockHint;

    // ── Story slides — shown before the level activates ───────────────────────
    QStringList   storyPages;
    QString       enterPrompt = "► CONTINUE";
};

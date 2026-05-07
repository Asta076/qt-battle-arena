#pragma once
#include <QString>
#include <QStringList>
#include "character.h"

// ─────────────────────────────────────────────────────────────────────────────
//  LevelDef — pure data, no Qt, no dependencies beyond character.h
//  Add a new level by adding a row to LevelManager::buildLevels()
// ─────────────────────────────────────────────────────────────────────────────
struct LevelDef {
    int           id;
    QString       name;

    // ── Boss identity ─────────────────────────────────────────────────────────
    QString       bossName;
    CharacterType bossType;
    int           bossHpMultiplier;    // e.g. 2 = double base HP
    int           bossAttackBonus;     // flat attack bonus on top of base
    int           bossGoldReward;

    // ── Unlock criteria — all must be satisfied ───────────────────────────────
    int           requiredRuns;        // dungeon runs needed  (0 = always open)
    int           requiredGold;        // gold threshold       (0 = no requirement)
    int           requiredLevelId;     // must beat this level first (0 = none)

    // ── Boss dialogue — shown before/after the fight ──────────────────────────
    QString       bossIntroLine;       // boss taunts player before fight
    QString       bossDefeatLine;      // boss says on losing
    QString       bossVictoryLine;     // boss says if player loses
    QString       unlockHint;          // shown on locked zone: "Requires: ..."

    // ── Story slides — shown before the level activates ───────────────────────
    // Each QString is one full page of narrative text.
    // Leave empty for no intro slide (level activates immediately).
    QStringList   storyPages;

    // ── Enter button label on the last story page ─────────────────────────────
    QString       enterPrompt = "► CONTINUE";
};

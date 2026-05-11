#include "levelmanager.h"

LevelManager::LevelManager()
{
    buildLevels();
}

void LevelManager::buildLevels()
{
    // ── Level 1 — Goblin Cave ─────────────────────────────────────────────────
    m_levels.append({
        .id               = 1,
        .name             = "Goblin Cave",
        .theme            = LevelTheme::Cave,

        .bossName         = "Grunt Chief",
        .bossType         = CharacterType::Warrior,
        .bossHpMultiplier = 2,
        .bossAttackBonus  = 5,
        .bossGoldReward   = 150,

        .requiredRuns     = 0,
        .requiredGold     = 0,
        .requiredLevelId  = 0,

        .bossIntroLine   = "Another fool from the dungeon.\n\n"
                           "You're brave — I'll give you that.\n\n"
                           "Let's see if brave is enough.",

        .bossDefeatLine  = "Stronger than I expected.\n\n"
                           "The one who hired me came from the dead forest east.\n\n"
                           "And they already knew your name.",

        .bossVictoryLine = "Not your day, wanderer.\n\n"
                           "Come back stronger.",

        .unlockHint      = "",

        .storyPages = {
            "The dungeon is behind you.\n\n"
            "Something about it felt arranged.\n\n"
            "Like someone knew you were coming.",

            "To the north — a cave.\n\n"
            "Goblins. Armed. Organized.\n\n"
            "Goblins don't organize themselves.",

            "Whoever hired them knows you.\n\n"
            "Time to find out who."
        },

        .enterPrompt = "► ENTER THE CAVE"
    });

    // ── Level 2 — Haunted Forest ──────────────────────────────────────────────
    m_levels.append({
        .id               = 2,
        .name             = "Haunted Forest",
        .theme            = LevelTheme::Forest,

        .bossName         = "Shadow Walker",
        .bossType         = CharacterType::Archer,
        .bossHpMultiplier = 2,
        .bossAttackBonus  = 8,
        .bossGoldReward   = 250,

        .requiredRuns     = 3,
        .requiredGold     = 0,
        .requiredLevelId  = 1,

        .bossIntroLine   = "I've watched every fight since the dungeon.\n\n"
                           "You're not special. Just persistent.\n\n"
                           "My employer thinks you're dangerous.\n\n"
                           "I'm starting to agree.",

        .bossDefeatLine  = "Fine. One name: Veyrath.\n\n"
                           "Frozen Peak. He's building something that shouldn't exist.\n\n"
                           "Go. And good luck — you'll need it.",

        .bossVictoryLine = "This forest breaks stronger heroes.\n\n"
                           "Veyrath will be waiting.\n\n"
                           "If you ever make it out.",

        .unlockHint      = "Beat Level 1 and complete 3 dungeon runs",

        .storyPages = {
            "Grunt Chief's last words:\n\n"
            "\"She's been watching.\n"
            "Since before you entered the dungeon.\"",

            "Outside, the sun was gone.\n\n"
            "The forest ahead looked like something\n"
            "that had forgotten how to be alive.",

            "The trail leads in.\n\n"
            "You step forward.\n\n"
            "Behind you, the cave collapses.\n\n"
            "No going back."
        },

        .enterPrompt = "► ENTER THE FOREST"
    });

    // ── Level 3 — Frozen Peak ─────────────────────────────────────────────────
    m_levels.append({
        .id               = 3,
        .name             = "Frozen Peak",
        .theme            = LevelTheme::Peak,

        .bossName         = "Veyrath",
        .bossType         = CharacterType::Mage,
        .bossHpMultiplier = 3,
        .bossAttackBonus  = 10,
        .bossGoldReward   = 400,

        .requiredRuns     = 6,
        .requiredGold     = 200,
        .requiredLevelId  = 2,

        .bossIntroLine   = "You climbed all this way.\n\n"
                           "Through everything I sent to stop you.\n\n"
                           "What I am building here will not be stopped\n"
                           "by someone like you.",

        .bossDefeatLine  = "I was building a door.\n\n"
                           "To somewhere sealed for good reason.\n\n"
                           "I thought I could control what came through.\n\n"
                           "Stop it. Before it opens.",

        .bossVictoryLine = "Your warmth fades here.\n\n"
                           "Everything does, eventually.\n\n"
                           "Go. Before the cold decides for you.",

        .unlockHint      = "Beat Level 2, complete 6 dungeon runs, earn 200 Gold",

        .storyPages = {
            "Shadow Walker said one word\n"
            "before she disappeared into the trees.\n\n"
            "Veyrath.",

            "The peak rose above the clouds.\n\n"
            "The ice here was old. Ancient.\n\n"
            "And it moved.",

            "Veyrath was building a door.\n\n"
            "You had to reach the top\n"
            "before whatever was on the other side\n"
            "finished knocking."
        },

        .enterPrompt = "► CLIMB THE PEAK"
    });

    // ── Level 4 — Volcanic Dungeon ────────────────────────────────────────────
    m_levels.append({
        .id               = 4,
        .name             = "Volcanic Dungeon",
        .theme            = LevelTheme::Volcano,

        .bossName         = "Ember Warrior",
        .bossType         = CharacterType::Warrior,
        .bossHpMultiplier = 3,
        .bossAttackBonus  = 15,
        .bossGoldReward   = 600,

        .requiredRuns     = 10,
        .requiredGold     = 0,
        .requiredLevelId  = 3,

        .bossIntroLine   = "The volcano burns away everything weak.\n\n"
                           "You're still standing after all of it.\n\n"
                           "Good. I was tired of waiting for someone worth fighting.",

        .bossDefeatLine  = "The fire dies... as do I.\n\n"
                           "What Veyrath opened is already here.\n\n"
                           "The Final Arena is not a place. It's a choice.\n\n"
                           "Choose carefully.",

        .bossVictoryLine = "You will be ash before the summit.\n\n"
                           "Come back with more fire than the mountain.\n\n"
                           "Or don't come back at all.",

        .unlockHint      = "Beat Level 3 and complete 10 dungeon runs",

        .storyPages = {
            "Veyrath's last words stayed with you.\n\n"
            "A door. Something that shouldn't exist.\n\n"
            "And the peak had already started to crack.",

            "The volcano had no name on any map.\n\n"
            "The people nearby called it The Waiting.\n\n"
            "Nobody asked what it was waiting for.",

            "At the center, something burned\n"
            "that was not fire.\n\n"
            "And it had been expecting you."
        },

        .enterPrompt = "► DESCEND INTO THE VOLCANO"
    });

    // ── Level 5 — The Final Arena ─────────────────────────────────────────────
    m_levels.append({
        .id               = 5,
        .name             = "The Final Arena",
        .theme            = LevelTheme::Arena,

        .bossName         = "The Champion",
        .bossType         = CharacterType::Warrior,   // overridden to player class at runtime
        .bossHpMultiplier = 4,
        .bossAttackBonus  = 20,
        .bossGoldReward   = 1000,

        .requiredRuns     = 0,
        .requiredGold     = 0,
        .requiredLevelId  = 4,

        .bossIntroLine   = "I am every version of you that didn't make it.\n\n"
                           "Every choice that went wrong.\n\n"
                           "Prove me wrong.",

        .bossDefeatLine  = "You have surpassed all of us.\n\n"
                           "The door is closed. Whatever came through went back.\n\n"
                           "It turns out the answer was someone\n"
                           "who simply refused to stop.\n\n"
                           "Well fought.",

        .bossVictoryLine = "You were not ready. Not yet.\n\n"
                           "But you came further than anyone expected.\n\n"
                           "The arena will be here.\n\n"
                           "It always is.",

        .unlockHint      = "Beat all 4 levels",

        .storyPages = {
            "Four worlds. Four bosses.\n\n"
            "A cave. A forest. A peak. A volcano.\n\n"
            "All of it leading here.",

            "The arena appeared without warning.\n\n"
            "No entrance. No gate.\n\n"
            "One moment you were walking.\n"
            "The next, you were already inside.",

            "Across the sand, a figure stood\n"
            "with your posture. Your weapon. Your stance.\n\n"
            "It turned.\n\n"
            "It had your face."
        },

        .enterPrompt = "► FACE YOUR SHADOW"
    });
}

// ─────────────────────────────────────────────────────────────────────────────

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
    if (l->requiredRuns    > 0 && p.dungeonRuns < l->requiredRuns)                 return false;
    if (l->requiredGold    > 0 && p.gold        < l->requiredGold)                 return false;
    if (l->requiredLevelId > 0 && !p.completedLevels.contains(l->requiredLevelId)) return false;
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

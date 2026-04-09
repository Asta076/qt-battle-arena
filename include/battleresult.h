#pragma once
#include <QString>

struct BattleResult {
    QString attackerName;
    QString defenderName;
    int     damageDone    = 0;
    bool    usedSpecial   = false;
    bool    defenderFainted = false;
};

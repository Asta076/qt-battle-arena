#pragma once

#include <QRectF>
#include <QString>
#include <QWidget>

#include "character.h"
#include "direction.h"
#include "healthbarwidget.h"

class QLabel;
class QVBoxLayout;
class QHBoxLayout;

class ArenaHelpers
{
public:
    ArenaHelpers() = delete;

    static QString movementSheetFor(CharacterType type);
    static QString attackSheetFor(CharacterType type);

    static QRectF playerHurtBox(const QRectF& playerRect);
    static QRectF enemyMeleeBox(const QRectF& enemyRect);

    static void buildPlayerHud(QWidget* owner,
                               QWidget*& hud,
                               QLabel*& healthLabel,
                               HealthBarWidget*& healthBar,
                               QLabel*& specialLabel,
                               HealthBarWidget*& specialBar,
                               void (*positionCallback)());

    static void updatePlayerHud(Character* player,
                                float specialPercent,
                                HealthBarWidget* healthBar,
                                HealthBarWidget* specialBar);

    static void positionBottomRight(QWidget* owner,
                                    QWidget* hud,
                                    int margin = 16);
};

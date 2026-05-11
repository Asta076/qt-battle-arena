#include "arenahelpers.h"

#include <QColor>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

QString ArenaHelpers::movementSheetFor(CharacterType type)
{
    switch (type) {
    case CharacterType::Warrior:
        return ":/sprites/warrior_movement_8dir_6frames.png";
    case CharacterType::Mage:
        return ":/sprites/mage_movement_8dir_6frames.png";
    case CharacterType::Archer:
        return ":/sprites/archer_movement_8dir_6frames.png";
    }

    return ":/sprites/archer_movement_8dir_6frames.png";
}

QString ArenaHelpers::attackSheetFor(CharacterType type)
{
    switch (type) {
    case CharacterType::Warrior:
        return ":/sprites/warrior_attack_8dir_7frames.png";
    case CharacterType::Mage:
        return ":/sprites/mage_attack_8dir_7frames.png";
    case CharacterType::Archer:
        return ":/sprites/archer_attack_8dir_7frames.png";
    }

    return ":/sprites/archer_attack_8dir_7frames.png";
}

QRectF ArenaHelpers::playerHurtBox(const QRectF& playerRect)
{
    return playerRect.adjusted(28, 28, -28, -12);
}

QRectF ArenaHelpers::enemyMeleeBox(const QRectF& enemyRect)
{
    return enemyRect.adjusted(14, 14, -14, -8);
}

void ArenaHelpers::buildPlayerHud(QWidget* owner,
                                  QWidget*& hud,
                                  QLabel*& healthLabel,
                                  HealthBarWidget*& healthBar,
                                  QLabel*& specialLabel,
                                  HealthBarWidget*& specialBar,
                                  void (*positionCallback)())
{
    if (hud || !owner)
        return;

    hud = new QWidget(owner);
    hud->setFixedSize(250, 78);
    hud->setAttribute(Qt::WA_StyledBackground, true);
    hud->setStyleSheet(
        "QWidget { background-color: rgba(13,13,26,190); border: 3px solid #F0E8D0; }"
        "QLabel  { background: transparent; border: none; color: #F0E8D0; font-size: 8px; }");

    auto* root = new QVBoxLayout(hud);
    root->setContentsMargins(10, 8, 10, 8);
    root->setSpacing(6);

    auto* healthRow = new QHBoxLayout;
    healthRow->setSpacing(8);

    healthLabel = new QLabel("HP", hud);
    healthLabel->setFixedWidth(60);

    healthBar = new HealthBarWidget(hud);
    healthBar->setFixedSize(160, 18);

    healthRow->addWidget(healthLabel);
    healthRow->addWidget(healthBar);

    auto* specialRow = new QHBoxLayout;
    specialRow->setSpacing(8);

    specialLabel = new QLabel("SPECIAL", hud);
    specialLabel->setFixedWidth(60);

    specialBar = new HealthBarWidget(hud);
    specialBar->setFixedSize(160, 18);
    specialBar->setFixedBarColor(QColor("#4A90D9"));

    specialRow->addWidget(specialLabel);
    specialRow->addWidget(specialBar);

    root->addLayout(healthRow);
    root->addLayout(specialRow);

    if (positionCallback)
        positionCallback();

    hud->raise();
    hud->show();
}

void ArenaHelpers::updatePlayerHud(Character* player,
                                   float specialPercent,
                                   HealthBarWidget* healthBar,
                                   HealthBarWidget* specialBar)
{
    if (!healthBar || !specialBar)
        return;

    if (!player) {
        healthBar->setBarPercent(0.0f);
        specialBar->setBarPercent(0.0f);
        return;
    }

    healthBar->setBarPercent(player->getHealthPercent());
    specialBar->setBarPercent(specialPercent);
}

void ArenaHelpers::positionBottomRight(QWidget* owner, QWidget* hud, int margin)
{
    if (!owner || !hud)
        return;

    hud->move(
        owner->width() - hud->width() - margin,
        owner->height() - hud->height() - margin
    );

    hud->raise();
}

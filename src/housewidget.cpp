#include "housewidget.h"
#include "playerprofile.h" // FIX 1: We must include this so the compiler knows what PlayerProfile and ItemType are!
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDebug>

HouseWidget::HouseWidget(AudioManager* audio, QWidget* parent) : QWidget(parent), m_audio(audio)
{
    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(16);

    // Title
    m_label = new QLabel("— PLAYER HOUSE —", this);
    m_label->setObjectName("titleLabel");
    m_label->setAlignment(Qt::AlignCenter);

    // Stats
    m_statsLabel = new QLabel("Stats:", this);
    m_statsLabel->setAlignment(Qt::AlignCenter);

    // Inventory
    m_inventoryLabel = new QLabel("Inventory:", this);
    m_inventoryLabel->setAlignment(Qt::AlignCenter);

    // Back button
    m_backButton = new QPushButton("← BACK", this);
    connect(m_backButton, &QPushButton::clicked, this, &HouseWidget::backToOverworld);

    // Layout
    layout->addStretch();
    layout->addWidget(m_label);
    layout->addWidget(m_statsLabel);
    layout->addWidget(m_inventoryLabel);
    layout->addSpacing(24);
    layout->addWidget(m_backButton, 0, Qt::AlignCenter);
    layout->addStretch();
}

// FIX 2: We removed the standalone getItemName method so you don't have to edit housewidget.h!

void HouseWidget::setProfile(PlayerProfile* profile)
{
    m_profile = profile;
    if (!m_profile) return;

    // Stats section
    QString statsText = QString(
                            "Stats:\n\n"
                            "Name: %1\n"
                            "Gold: %2\n"
                            "Dungeon Runs: %3\n"
                            "Bonus HP: %4\n"
                            "Bonus ATK: %5"
                            ).arg(m_profile->characterName)
                            .arg(m_profile->gold)
                            .arg(m_profile->dungeonRuns)
                            .arg(m_profile->upgrades.bonusMaxHp)
                            .arg(m_profile->upgrades.bonusAttack);

    m_statsLabel->setText(statsText);

    // Inventory section
    QString invText = "Inventory:\n\n";
    bool hasItems = false;

    for (auto it = m_profile->inventory.begin(); it != m_profile->inventory.end(); ++it) {
        if (it.value() > 0) {
            hasItems = true;

            // We moved the switch statement directly into the loop!
            QString itemName;
            switch (it.key()) {
            case ItemType::HealthPotion: itemName = "Health Potion"; break;
            case ItemType::SpPotion:     itemName = "SP Potion"; break;
            case ItemType::AttackBoost:  itemName = "Attack Boost"; break;
            case ItemType::DefenseBoost: itemName = "Defense Shield"; break;
            default:                     itemName = "Unknown Item"; break;
            }

            invText += QString("- %1 x%2\n")
                           .arg(itemName)
                           .arg(it.value());
        }
    }

    if (!hasItems) {
        invText += " (Your pockets are empty!)";
    }

    m_inventoryLabel->setText(invText);
}

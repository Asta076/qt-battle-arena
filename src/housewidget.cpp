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

    // We use HTML tags to add colors, bolding, and clean sizing!
    QString statsText = QString(
                            "<div align='center'>"
                            "<h2 style='color: #4da6ff;'>Player Stats</h2>"
                            "<span style='font-size: 16px;'>"
                            "<b>Name:</b> %1<br>"
                            "<b>Gold:</b> <span style='color: #FFD700;'>%2 G</span><br>"
                            "<b>Dungeon Runs:</b> %3<br>"
                            "<b>Bonus HP:</b> <span style='color: #ff4d4d;'>+%4</span><br>"
                            "<b>Bonus ATK:</b> <span style='color: #ff9933;'>+%5</span>"
                            "</span>"
                            "</div>"
                            ).arg(m_profile->characterName)
                            .arg(m_profile->gold)
                            .arg(m_profile->dungeonRuns)
                            .arg(m_profile->upgrades.bonusMaxHp)
                            .arg(m_profile->upgrades.bonusAttack);

    m_statsLabel->setText(statsText);

    // Style the inventory section similarly
    QString invText = "<div align='center'>"
                      "<h2 style='color: #66cc66;'>Inventory</h2>"
                      "<span style='font-size: 16px;'>";
    bool hasItems = false;

    for (auto it = m_profile->inventory.begin(); it != m_profile->inventory.end(); ++it) {
        if (it.value() > 0) {
            hasItems = true;

            QString itemName;
            switch (it.key()) {
            case ItemType::HealthPotion: itemName = "Health Potion"; break;
            case ItemType::SpPotion:     itemName = "SP Potion"; break;
            case ItemType::AttackBoost:  itemName = "Attack Boost"; break;
            case ItemType::DefenseBoost: itemName = "Defense Shield"; break;
            default:                     itemName = "Unknown Item"; break;
            }

            invText += QString("<b>%1:</b> x%2<br>")
                           .arg(itemName)
                           .arg(it.value());
        }
    }

    if (!hasItems) {
        // Make the empty message italic and gray so it looks intentional
        invText += "<i style='color: gray;'>(Your pockets are empty!)</i>";
    }

    invText += "</span></div>";

    m_inventoryLabel->setText(invText);
}

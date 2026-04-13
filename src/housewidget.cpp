#include "housewidget.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDebug>

#include "playerprofile.h"

HouseWidget::HouseWidget(AudioManager* audio, QWidget* parent)
    : QWidget(parent), m_audio(audio)
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
    connect(m_backButton, &QPushButton::clicked,
            this, &HouseWidget::backToOverworld);

    // Layout
    layout->addStretch();
    layout->addWidget(m_label);
    layout->addWidget(m_statsLabel);
    layout->addWidget(m_inventoryLabel);
    layout->addSpacing(24);
    layout->addWidget(m_backButton, 0, Qt::AlignCenter);
    layout->addStretch();
}

void HouseWidget::setProfile(PlayerProfile* profile)
{
    m_profile = profile;

    if (!m_profile)
        return;

    qDebug() << "Gold:" << m_profile->gold;
    qDebug() << "Runs:" << m_profile->dungeonRuns;
    qDebug() << "Inventory size:" << m_profile->inventory.size();

    // Stats
    QString statsText = QString("Gold: %1\nDungeon Runs: %2")
                            .arg(m_profile->gold)
                            .arg(m_profile->dungeonRuns);

    m_statsLabel->setText(statsText);

    // Inventory
    QString invText = "Inventory:\n";

    for (auto it = m_profile->inventory.begin();
         it != m_profile->inventory.end();
         ++it)
    {
        if (it.value() > 0)
        {
            invText += QString("- Item %1 x%2\n")
                           .arg(static_cast<int>(it.key()))
                           .arg(it.value());
        }
    }

    m_inventoryLabel->setText(invText);
}

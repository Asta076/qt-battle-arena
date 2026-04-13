#include "housewidget.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPainter>

HouseWidget::HouseWidget(AudioManager* audio, QWidget* parent)
    : QWidget(parent), m_audio(audio)
{
    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(16);

    m_label = new QLabel("— PLAYER HOUSE —", this);
    m_label->setObjectName("titleLabel");
    m_label->setAlignment(Qt::AlignCenter);

    auto* sub = new QLabel("Inventory coming soon...", this);
    sub->setObjectName("subtitleLabel");
    sub->setAlignment(Qt::AlignCenter);

    m_backButton = new QPushButton("← BACK", this);
    connect(m_backButton, &QPushButton::clicked,
            this, &HouseWidget::backToOverworld);

    layout->addStretch();
    layout->addWidget(m_label);
    layout->addWidget(sub);
    layout->addSpacing(24);
    layout->addWidget(m_backButton, 0, Qt::AlignCenter);
    layout->addStretch();
}
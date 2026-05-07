#include "pvpcharacterselectwidget.h"

#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QVariant>

PvpCharacterSelectWidget::PvpCharacterSelectWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("pvpCharacterSelect");

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setAlignment(Qt::AlignCenter);
    root->setSpacing(22);

    QLabel* title = new QLabel("PVP CHARACTER SELECT", this);
    title->setAlignment(Qt::AlignCenter);
    title->setFont(QFont("Arial", 28, QFont::Bold));

    QLabel* subtitle = new QLabel("Choose a class for each player", this);
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setFont(QFont("Arial", 13));

    QLabel* p1Label = new QLabel("Player 1", this);
    p1Label->setAlignment(Qt::AlignCenter);
    p1Label->setFont(QFont("Arial", 16, QFont::Bold));

    QLabel* p2Label = new QLabel("Player 2", this);
    p2Label->setAlignment(Qt::AlignCenter);
    p2Label->setFont(QFont("Arial", 16, QFont::Bold));

    m_p1Combo = new QComboBox(this);
    m_p2Combo = new QComboBox(this);

    auto addClasses = [](QComboBox* combo) {
        combo->addItem("Warrior", static_cast<int>(CharacterType::Warrior));
        combo->addItem("Mage",    static_cast<int>(CharacterType::Mage));
        combo->addItem("Archer",  static_cast<int>(CharacterType::Archer));
    };

    addClasses(m_p1Combo);
    addClasses(m_p2Combo);

    m_p1Combo->setCurrentIndex(0);
    m_p2Combo->setCurrentIndex(2);

    QHBoxLayout* playerRow = new QHBoxLayout;
    playerRow->setAlignment(Qt::AlignCenter);
    playerRow->setSpacing(40);

    QVBoxLayout* p1Box = new QVBoxLayout;
    p1Box->setAlignment(Qt::AlignCenter);
    p1Box->addWidget(p1Label);
    p1Box->addWidget(m_p1Combo);

    QVBoxLayout* p2Box = new QVBoxLayout;
    p2Box->setAlignment(Qt::AlignCenter);
    p2Box->addWidget(p2Label);
    p2Box->addWidget(m_p2Combo);

    playerRow->addLayout(p1Box);
    playerRow->addLayout(p2Box);

    m_startBtn = new QPushButton("START DUEL", this);
    m_backBtn  = new QPushButton("BACK", this);

    root->addStretch(2);
    root->addWidget(title);
    root->addWidget(subtitle);
    root->addStretch(1);
    root->addLayout(playerRow);
    root->addSpacing(20);
    root->addWidget(m_startBtn);
    root->addWidget(m_backBtn);
    root->addStretch(2);

    connect(m_startBtn, &QPushButton::clicked, this, [this]() {
        emit duelStartRequested(
            selectedTypeFromCombo(m_p1Combo),
            selectedTypeFromCombo(m_p2Combo)
        );
    });

    connect(m_backBtn, &QPushButton::clicked,
            this, &PvpCharacterSelectWidget::backRequested);
}

CharacterType PvpCharacterSelectWidget::selectedTypeFromCombo(QComboBox* combo) const
{
    return static_cast<CharacterType>(combo->currentData().toInt());
}

#include "pvpcharacterselectwidget.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariant>

PvpCharacterSelectWidget::PvpCharacterSelectWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("pvpCharacterSelect");
    setAutoFillBackground(false);

    m_background.load(":/backgrounds/pvp_arena.png");

    setStyleSheet(R"(
        QLabel#titleLabel {
            color: #F8F1D8;
            font-size: 34px;
            font-weight: bold;
        }

        QLabel#subtitleLabel {
            color: #FACC15;
            font-size: 15px;
            font-weight: bold;
        }

        QLabel#playerTitle {
            color: white;
            font-size: 22px;
            font-weight: bold;
        }

        QLabel#controlsLabel {
            color: #D1D5DB;
            font-size: 12px;
            font-weight: bold;
        }

        QFrame#playerPanel {
            background-color: rgba(10, 10, 20, 185);
            border: 3px solid #C89B3C;
            border-radius: 18px;
        }

        QComboBox {
            background-color: #1F2937;
            color: white;
            border: 2px solid #C89B3C;
            border-radius: 8px;
            padding: 8px;
            font-size: 16px;
            font-weight: bold;
            min-width: 210px;
        }

        QComboBox QAbstractItemView {
            background-color: #111827;
            color: white;
            selection-background-color: #C89B3C;
        }

        QPushButton {
            background-color: #7C2D12;
            color: #F8F1D8;
            border: 3px solid #FACC15;
            border-radius: 12px;
            padding: 10px 28px;
            font-size: 16px;
            font-weight: bold;
        }

        QPushButton:hover {
            background-color: #9A3412;
        }

        QPushButton:pressed {
            background-color: #431407;
        }
    )");

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setAlignment(Qt::AlignCenter);
    root->setContentsMargins(60, 40, 60, 40);
    root->setSpacing(20);

    QLabel* title = new QLabel("PVP CHARACTER SELECT", this);
    title->setObjectName("titleLabel");
    title->setAlignment(Qt::AlignCenter);

    QLabel* subtitle = new QLabel("Choose your fighters before entering the colosseum", this);
    subtitle->setObjectName("subtitleLabel");
    subtitle->setAlignment(Qt::AlignCenter);

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

    QFrame* p1Panel = new QFrame(this);
    p1Panel->setObjectName("playerPanel");
    p1Panel->setMinimumSize(300, 190);

    QVBoxLayout* p1Layout = new QVBoxLayout(p1Panel);
    p1Layout->setAlignment(Qt::AlignCenter);
    p1Layout->setSpacing(14);

    QLabel* p1Label = new QLabel("PLAYER 1", p1Panel);
    p1Label->setObjectName("playerTitle");
    p1Label->setAlignment(Qt::AlignCenter);

    QLabel* p1Controls = new QLabel("Controls: WASD", p1Panel);
    p1Controls->setObjectName("controlsLabel");
    p1Controls->setAlignment(Qt::AlignCenter);

    p1Layout->addWidget(p1Label);
    p1Layout->addWidget(m_p1Combo);
    p1Layout->addWidget(p1Controls);

    QFrame* p2Panel = new QFrame(this);
    p2Panel->setObjectName("playerPanel");
    p2Panel->setMinimumSize(300, 190);

    QVBoxLayout* p2Layout = new QVBoxLayout(p2Panel);
    p2Layout->setAlignment(Qt::AlignCenter);
    p2Layout->setSpacing(14);

    QLabel* p2Label = new QLabel("PLAYER 2", p2Panel);
    p2Label->setObjectName("playerTitle");
    p2Label->setAlignment(Qt::AlignCenter);

    QLabel* p2Controls = new QLabel("Controls: Arrow Keys", p2Panel);
    p2Controls->setObjectName("controlsLabel");
    p2Controls->setAlignment(Qt::AlignCenter);

    p2Layout->addWidget(p2Label);
    p2Layout->addWidget(m_p2Combo);
    p2Layout->addWidget(p2Controls);

    QHBoxLayout* playerRow = new QHBoxLayout;
    playerRow->setAlignment(Qt::AlignCenter);
    playerRow->setSpacing(50);
    playerRow->addWidget(p1Panel);
    playerRow->addWidget(p2Panel);

    m_startBtn = new QPushButton("START DUEL", this);
    m_backBtn  = new QPushButton("BACK", this);

    m_startBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setCursor(Qt::PointingHandCursor);

    QHBoxLayout* buttonRow = new QHBoxLayout;
    buttonRow->setAlignment(Qt::AlignCenter);
    buttonRow->setSpacing(18);
    buttonRow->addWidget(m_startBtn);
    buttonRow->addWidget(m_backBtn);

    root->addStretch(1);
    root->addWidget(title);
    root->addWidget(subtitle);
    root->addSpacing(20);
    root->addLayout(playerRow);
    root->addSpacing(18);
    root->addLayout(buttonRow);
    root->addStretch(1);

    connect(m_startBtn, &QPushButton::clicked, this, [this]() {
        emit duelStartRequested(
            selectedTypeFromCombo(m_p1Combo),
            selectedTypeFromCombo(m_p2Combo)
            );
    });

    connect(m_backBtn, &QPushButton::clicked,
            this, &PvpCharacterSelectWidget::backRequested);
}

void PvpCharacterSelectWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    painter.fillRect(rect(), QColor("#050816"));

    if (!m_background.isNull()) {
        painter.drawPixmap(rect(), m_background);
    }

    painter.fillRect(rect(), QColor(0, 0, 0, 145));
}

CharacterType PvpCharacterSelectWidget::selectedTypeFromCombo(QComboBox* combo) const
{
    return static_cast<CharacterType>(combo->currentData().toInt());
}

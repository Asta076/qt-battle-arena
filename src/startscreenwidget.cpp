#include "startscreenwidget.h"
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>

StartScreenWidget::StartScreenWidget(GameEngine* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    // ── Layout ────────────────────────────────────
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setAlignment(Qt::AlignCenter);
    root->setSpacing(24);

    // Title
    QLabel* title = new QLabel("CSCE 1101\nBATTLE ARENA", this);
    title->setObjectName("titleLabel");
    title->setAlignment(Qt::AlignCenter);

    // Subtitle / version
    QLabel* sub = new QLabel("v1.0  —  Spring 2026", this);
    sub->setObjectName("subtitleLabel");
    sub->setAlignment(Qt::AlignCenter);

    // Difficulty row
    QLabel* diffLabel = new QLabel("DIFFICULTY:", this);
    m_difficultyBox = new QComboBox(this);
    m_difficultyBox->addItem("EASY",   QVariant::fromValue(Difficulty::Easy));
    m_difficultyBox->addItem("NORMAL", QVariant::fromValue(Difficulty::Normal));
    m_difficultyBox->addItem("HARD",   QVariant::fromValue(Difficulty::Hard));
    m_difficultyBox->setCurrentIndex(1); // default Normal

    QHBoxLayout* diffRow = new QHBoxLayout;
    diffRow->setAlignment(Qt::AlignCenter);
    diffRow->addWidget(diffLabel);
    diffRow->addWidget(m_difficultyBox);

    // Buttons
    m_startBtn = new QPushButton("► START GAME", this);
    m_pvpBtn   = new QPushButton("  PVP BATTLE", this);
    m_loadBtn  = new QPushButton("  LOAD SAVE",  this);
    m_onlinePvpBtn = new QPushButton("  ONLINE PVP", this);

    // Assemble
    root->addStretch(2);
    root->addWidget(title);
    root->addWidget(sub);
    root->addStretch(1);
    root->addLayout(diffRow);
    root->addSpacing(12);
    root->addWidget(m_startBtn);
    root->addWidget(m_pvpBtn);
    root->addWidget(m_loadBtn);
    root->addWidget(m_onlinePvpBtn);

    root->addStretch(3);

    // ── Connections ───────────────────────────────
    connect(m_startBtn, &QPushButton::clicked,
            this, &StartScreenWidget::startRequested);

    connect(m_pvpBtn, &QPushButton::clicked,
            this, &StartScreenWidget::pvpRequested);

    connect(m_loadBtn, &QPushButton::clicked,
            this, &StartScreenWidget::loadRequested);
    connect(m_onlinePvpBtn, &QPushButton::clicked,
            this, &StartScreenWidget::onlinePvpRequested);
}

void StartScreenWidget::paintEvent(QPaintEvent* event)
{
    QPainter p(this);

    // Background
    QPixmap bg(":/backgrounds/battle_bg.png");
    if (!bg.isNull())
        p.drawPixmap(rect(), bg);
    else
        p.fillRect(rect(), QColor("#1A1A2E"));

    // Dark overlay so text stays readable over the BG
    p.fillRect(rect(), QColor(0, 0, 0, 120));

    QWidget::paintEvent(event);
}

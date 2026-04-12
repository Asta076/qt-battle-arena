#include "gameoverwidget.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPainter>

GameOverWidget::GameOverWidget(GameEngine* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(20);

    m_resultLabel = new QLabel(this);
    m_resultLabel->setObjectName("titleLabel");
    m_resultLabel->setAlignment(Qt::AlignCenter);

    m_scoreLabel = new QLabel(this);
    m_scoreLabel->setObjectName("subtitleLabel");
    m_scoreLabel->setAlignment(Qt::AlignCenter);

    QPushButton* explore  = new QPushButton("► EXPLORE MORE", this);
    QPushButton* scores   = new QPushButton("  SCOREBOARD",   this);
    QPushButton* mainMenu = new QPushButton("  MAIN MENU",    this);

    connect(explore, &QPushButton::clicked, this, [this]{
        emit returnToOverworld();
    });
    connect(scores, &QPushButton::clicked, this, [engine]{
        engine->setState(GameState::Scoreboard);
    });
    connect(mainMenu, &QPushButton::clicked, engine, &GameEngine::onExitToMenu);

    connect(engine, &GameEngine::gameOver,
            this,   &GameOverWidget::onGameOver);

    layout->addStretch();
    layout->addWidget(m_resultLabel);
    layout->addWidget(m_scoreLabel);
    layout->addSpacing(24);
    layout->addWidget(explore);
    layout->addWidget(mainMenu);
    layout->addWidget(scores);
    layout->addStretch();
}

void GameOverWidget::onGameOver(bool playerWon, int pScore, int eScore)
{
    m_resultLabel->setText(playerWon ? "✦ VICTORY! ✦" : "✦ DEFEATED ✦");
    m_scoreLabel->setText(QString("Final Score:  %1 — %2").arg(pScore).arg(eScore));
}

void GameOverWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    QPixmap bg(":/backgrounds/battle_bg.png");
    if (!bg.isNull()) p.drawPixmap(rect(), bg);
    else p.fillRect(rect(), QColor("#1A1A2E"));
    p.fillRect(rect(), QColor(0, 0, 0, 160));
}

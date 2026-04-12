#include "pauseoverlaywidget.h"
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QPainter>

PauseOverlayWidget::PauseOverlayWidget(GameEngine* engine, QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(16);

    QLabel* title    = new QLabel("— PAUSED —",  this);
    title->setObjectName("titleLabel");
    title->setAlignment(Qt::AlignCenter);

    QPushButton* resumeBtn = new QPushButton("► RESUME",    this);
    QPushButton* menuBtn   = new QPushButton("  MAIN MENU", this);

    connect(resumeBtn, &QPushButton::clicked, engine, &GameEngine::onPauseToggle);
    connect(menuBtn,   &QPushButton::clicked, engine, &GameEngine::onExitToMenu);

    layout->addWidget(title);
    layout->addSpacing(12);
    layout->addWidget(resumeBtn);
    layout->addWidget(menuBtn);
    // No save button — saving only happens in overworld pause
}

void PauseOverlayWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0, 180));
}
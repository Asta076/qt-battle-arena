#include "pauseoverlaywidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPainter>

PauseOverlayWidget::PauseOverlayWidget(bool showSave, QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    hide();

    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(16);

    auto* title  = new QLabel("— PAUSED —", this);
    auto* resume = new QPushButton("► RESUME", this);
    
    title->setObjectName("titleLabel");
    title->setAlignment(Qt::AlignCenter);

    connect(resume, &QPushButton::clicked, this, &PauseOverlayWidget::resumeRequested);

    layout->addWidget(title);
    layout->addSpacing(12);
    layout->addWidget(resume);

    if (showSave) {
        auto* save = new QPushButton("  SAVE GAME", this);
        connect(save, &QPushButton::clicked, this, &PauseOverlayWidget::saveRequested);
        layout->addWidget(save);
    }

    auto* menu = new QPushButton("  MAIN MENU", this);
    connect(menu, &QPushButton::clicked, this, &PauseOverlayWidget::menuRequested);
    layout->addWidget(menu);
}

void PauseOverlayWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0, 180));
}

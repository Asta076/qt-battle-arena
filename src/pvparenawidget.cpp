#include "pvparenawidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QFont>
#include <QPen>

PvpArenaWidget::PvpArenaWidget(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);

    m_arenaBackground.load(":/backgrounds/pvp_arena.png");
}

void PvpArenaWidget::activate()
{
    setFocus(Qt::OtherFocusReason);
    update();
}

void PvpArenaWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // Fallback background if the PNG does not load
    painter.fillRect(rect(), QColor("#111827"));

    if (!m_arenaBackground.isNull()) {
        painter.drawPixmap(rect(), m_arenaBackground);
    }

    // Temporary controls text
    painter.setPen(QColor("#F9FAFB"));
    painter.setFont(QFont("Arial", 14, QFont::Bold));

    QRect controlsRect(0, height() - 45, width(), 30);

    painter.fillRect(
        controlsRect.adjusted(0, -5, 0, 5),
        QColor(0, 0, 0, 130)
        );

    painter.drawText(
        controlsRect,
        Qt::AlignCenter,
        "P1: WASD    |    P2: Arrow Keys    |    ESC: Back"
        );
}

void PvpArenaWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        emit backToMenu();
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

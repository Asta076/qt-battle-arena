#include "pvparenawidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QFont>

PvpArenaWidget::PvpArenaWidget(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
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
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Background
    painter.fillRect(rect(), QColor("#111827"));

    // Arena box
    QRect arenaRect = rect().adjusted(60, 60, -60, -60);
    painter.setPen(QPen(QColor("#FACC15"), 4));
    painter.setBrush(QColor("#1F2937"));
    painter.drawRoundedRect(arenaRect, 20, 20);

    // Title
    painter.setPen(QColor("#F9FAFB"));
    painter.setFont(QFont("Arial", 32, QFont::Bold));
    painter.drawText(rect(), Qt::AlignCenter, "PVP ARENA");

    // Controls text
    painter.setFont(QFont("Arial", 14));
    painter.drawText(
        QRect(0, height() - 80, width(), 40),
        Qt::AlignCenter,
        "ESC: Back to Menu"
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

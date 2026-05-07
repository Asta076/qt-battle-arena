#include "pvparenawidget.h"

#include <QColor>
#include <QFont>
#include <QKeyEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QString>

PvpArenaWidget::PvpArenaWidget(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);

    m_arenaBackground.load(":/backgrounds/pvp_arena.png");
    m_playerSprite.load(":/sprites/player.png");

    resetPlayers();
}

void PvpArenaWidget::activate()
{
    resetPlayers();
    setFocus(Qt::OtherFocusReason);
    update();
}

void PvpArenaWidget::resetPlayers()
{
    // World coordinates, not screen pixels.
    // P1 starts left side, P2 starts right side.
    m_p1Pos = QPointF(330.0, 430.0);
    m_p2Pos = QPointF(580.0, 430.0);
}

void PvpArenaWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // Fill outside the arena area if the window is not exactly 4:3.
    painter.fillRect(rect(), QColor("#050816"));

    // Keep the arena in a 4:3 world so it does not stretch weirdly.
    const qreal scale = qMin(width() / WORLD_W, height() / WORLD_H);
    const qreal xOffset = (width() - WORLD_W * scale) / 2.0;
    const qreal yOffset = (height() - WORLD_H * scale) / 2.0;

    painter.save();
    painter.translate(xOffset, yOffset);
    painter.scale(scale, scale);

    // Arena background.
    if (!m_arenaBackground.isNull()) {
        painter.drawPixmap(
            QRect(0, 0, static_cast<int>(WORLD_W), static_cast<int>(WORLD_H)),
            m_arenaBackground
            );
    } else {
        painter.fillRect(QRectF(0, 0, WORLD_W, WORLD_H), QColor("#111827"));

        painter.setPen(QPen(QColor("#FACC15"), 4));
        painter.setBrush(QColor("#1F2937"));
        painter.drawRoundedRect(QRectF(80, 80, WORLD_W - 160, WORLD_H - 160), 20, 20);
    }

    // Players.
    drawPlayer(painter, m_p1Pos, "P1", QColor("#3A86FF"));
    drawPlayer(painter, m_p2Pos, "P2", QColor("#FF4D6D"));

    painter.restore();

    // Controls text overlay.
    QRect controlsRect(0, height() - 46, width(), 34);

    painter.fillRect(
        controlsRect.adjusted(0, -6, 0, 6),
        QColor(0, 0, 0, 150)
        );

    painter.setPen(QColor("#F9FAFB"));
    painter.setFont(QFont("Arial", 14, QFont::Bold));
    painter.drawText(
        controlsRect,
        Qt::AlignCenter,
        "P1: WASD    |    P2: Arrow Keys    |    ESC: Back"
        );
}

void PvpArenaWidget::drawPlayer(QPainter& painter,
                                const QPointF& pos,
                                const QString& label,
                                const QColor& outlineColor)
{
    QRectF body(pos.x(), pos.y(), PLAYER_W, PLAYER_H);

    // Small shadow under player.
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 90));
    painter.drawEllipse(QRectF(body.x() + 6, body.y() + PLAYER_H - 10, PLAYER_W - 12, 12));

    if (!m_playerSprite.isNull()) {
        painter.drawPixmap(body.toRect(), m_playerSprite);
    } else {
        painter.setBrush(QColor("#E5E7EB"));
        painter.setPen(QPen(outlineColor, 3));
        painter.drawRoundedRect(body, 8, 8);
    }

    // Colored outline so P1 and P2 are easy to tell apart.
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(outlineColor, 3));
    painter.drawRoundedRect(body, 6, 6);

    // Label above player.
    painter.setPen(QColor("#FFFFFF"));
    painter.setFont(QFont("Arial", 13, QFont::Bold));
    painter.drawText(
        QRectF(body.x() - 20, body.y() - 26, body.width() + 40, 22),
        Qt::AlignCenter,
        label
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

#include "goldhudwidget.h"
#include <QPainter>
#include <QFont>

GoldHudWidget::GoldHudWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(140, 36);
}

void GoldHudWidget::setGold(int gold)
{
    m_gold = gold;
    update();
}

void GoldHudWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    const int w = width();
    const int h = height();

    // ── Dark background pill ──────────────────────────────────────────────────
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 180));
    p.drawRoundedRect(0, 0, w, h, 4, 4);

    // ── Outer border ──────────────────────────────────────────────────────────
    p.setPen(QPen(QColor("#F0E8D0"), 2));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(1, 1, w - 2, h - 2, 4, 4);

    // ── Coin: outer circle (dark border) ─────────────────────────────────────
    const int coinX = 8;
    const int coinY = (h - 20) / 2;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#1A1A2E"));
    p.drawEllipse(coinX, coinY, 20, 20);

    // ── Coin: yellow fill ─────────────────────────────────────────────────────
    p.setBrush(QColor("#FFD700"));
    p.drawEllipse(coinX + 2, coinY + 2, 16, 16);

    // ── Coin: inner highlight dot ─────────────────────────────────────────────
    p.setBrush(QColor("#FFEE88"));
    p.drawEllipse(coinX + 5, coinY + 4, 5, 5);

    // ── Gold number ───────────────────────────────────────────────────────────
    p.setPen(QColor("#FFE066"));
    p.setFont(QFont("Press Start 2P", 8));
    QRect textRect(coinX + 26, 0, w - coinX - 32, h);
    p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft,
               QString::number(m_gold));
}
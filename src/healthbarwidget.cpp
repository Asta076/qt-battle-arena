#include "healthbarwidget.h"
#include <QPainter>
#include <QPropertyAnimation>

HealthBarWidget::HealthBarWidget(QWidget* parent)
    : QWidget(parent)
{
    m_anim = new QPropertyAnimation(this, "healthPercent", this);
    m_anim->setDuration(400);
    setMinimumSize(160, 14);
}

void HealthBarWidget::setHealthPercent(float p)
{
    m_percent = std::clamp(p, 0.0f, 1.0f);
    update(); // triggers repaint
}

void HealthBarWidget::animateTo(float target)
{
    m_anim->stop();
    m_anim->setStartValue(m_percent);
    m_anim->setEndValue(std::clamp(target, 0.0f, 1.0f));
    m_anim->start();
}

void HealthBarWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false); // keep pixelated look

    const int w = width();
    const int h = height();

    // Outer border (white, 2px)
    p.fillRect(0, 0, w, h, QColor("#F0E8D0"));

    // Inner background (dark)
    p.fillRect(2, 2, w-4, h-4, QColor("#0D0D1A"));

    // Health fill
    int fillW = static_cast<int>((w - 4) * m_percent);
    QColor barColor = (m_percent > 0.5f) ? QColor("#44BB44")
                      : (m_percent > 0.25f)? QColor("#FFBB00")
                                            : QColor("#EE3333");
    if (fillW > 0)
        p.fillRect(2, 2, fillW, h-4, barColor);
}

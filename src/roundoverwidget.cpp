#include "roundoverwidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QPainter>
#include <QTimer>

RoundOverWidget::RoundOverWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    m_label = new QLabel(this);
    m_label->setObjectName("titleLabel");
    m_label->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_label);
}

void RoundOverWidget::show(const QString& message)
{
    m_label->setText(message);
    QWidget::show();
    // Auto-hide after 2 seconds (GameEngine will trigger state change anyway)
    QTimer::singleShot(2000, this, &RoundOverWidget::hide);
}

void RoundOverWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0, 160));
}

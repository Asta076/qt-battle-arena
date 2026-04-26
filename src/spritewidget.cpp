#include "spritewidget.h"
#include <QPainter>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QTimer>               // ← ADD THIS

SpriteWidget::SpriteWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    m_moveAnim = new QPropertyAnimation(this, "spriteOffset", this);
}

void SpriteWidget::setSprite(const QString& path)
{
    m_pixmap = SpriteCache::instance().get(path); 
    m_opacity  = 1.0f;
    m_fainting = false;
    m_offset   = {0, 0};
    update();
}

void SpriteWidget::setFallbackColor(const QColor& c, const QString& label)
{
    m_fallbackColor = c;
    m_fallbackLabel = label;
    m_opacity  = 1.0f;       // ← ADD
    m_fainting = false;      // ← ADD
    m_offset   = {0, 0};     // ← ADD
    update();
}

void SpriteWidget::playAttackAnimation(bool moveRight)
{
    int direction = moveRight ? 1 : -1;
    auto* seq = new QSequentialAnimationGroup(this);

    auto* forward = new QPropertyAnimation(this, "spriteOffset");
    forward->setDuration(150);
    forward->setStartValue(QPoint(0, 0));
    forward->setEndValue(QPoint(direction * 30, 0));

    auto* back = new QPropertyAnimation(this, "spriteOffset");
    back->setDuration(200);
    back->setStartValue(QPoint(direction * 30, 0));
    back->setEndValue(QPoint(0, 0));

    seq->addAnimation(forward);
    seq->addAnimation(back);
    seq->start(QAbstractAnimation::DeleteWhenStopped);
}

void SpriteWidget::playHitAnimation()
{
    auto* seq = new QSequentialAnimationGroup(this);

    // Shake: left-right-left-center
    auto makeShake = [this](int x) {
        auto* a = new QPropertyAnimation(this, "spriteOffset");
        a->setDuration(60);
        a->setStartValue(m_offset);
        a->setEndValue(QPoint(x, 0));
        return a;
    };
    seq->addAnimation(makeShake(-12));
    seq->addAnimation(makeShake( 12));
    seq->addAnimation(makeShake( -8));
    seq->addAnimation(makeShake(  0));
    seq->start(QAbstractAnimation::DeleteWhenStopped);
}

void SpriteWidget::playFaintAnimation()
{
    // Slide down and fade (we animate offset downward)
    auto* fall = new QPropertyAnimation(this, "spriteOffset");
    fall->setDuration(500);
    fall->setStartValue(QPoint(0, 0));
    fall->setEndValue(QPoint(0, height() / 2));
    fall->start(QAbstractAnimation::DeleteWhenStopped);

    // Fade via opacity (we'll add a simple timer-based fade)
    m_fainting = true;
    m_opacity  = 1.0f;

    QTimer* fadeTimer = new QTimer(this);
    connect(fadeTimer, &QTimer::timeout, this, [this, fadeTimer]{
        m_opacity -= 0.05f;
        update();
        if (m_opacity <= 0.0f) { m_opacity = 0.0f; fadeTimer->stop(); }
    });
    fadeTimer->start(30);
}

void SpriteWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    p.setOpacity(m_opacity);

    QRect drawRect = rect().translated(m_offset);

    if (!m_pixmap.isNull()) {
        if (m_mirrored) {
            QTransform t;
            t.translate(width() + m_offset.x(), m_offset.y());
            t.scale(-1, 1);
            p.setTransform(t);
            p.drawPixmap(QRect(0, 0, width(), height()),
                         m_pixmap.scaled(size(), Qt::KeepAspectRatio,
                                         Qt::FastTransformation));
        } else {
            p.drawPixmap(drawRect,
                         m_pixmap.scaled(size(), Qt::KeepAspectRatio,
                                         Qt::FastTransformation));
        }
    } else {
        // Fallback: colored block with initial letter
        p.fillRect(drawRect, m_fallbackColor);
        p.setPen(QColor("#F0E8D0"));
        p.setFont(QFont("Press Start 2P", 24));
        p.drawText(drawRect, Qt::AlignCenter,
                   m_fallbackLabel.isEmpty() ? "?" : m_fallbackLabel.left(1));
    }
}

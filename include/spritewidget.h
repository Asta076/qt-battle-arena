#pragma once
#include <QWidget>
#include <QPixmap>
#include <QColor>

class QPropertyAnimation;

class SpriteWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QPoint spriteOffset READ spriteOffset
                   WRITE setSpriteOffset)
public:
    explicit SpriteWidget(QWidget* parent = nullptr);

    void setSprite(const QString& resourcePath);
    void setFallbackColor(const QColor& c, const QString& label);
    void setMirrored(bool m) { m_mirrored = m; }

    void playAttackAnimation(bool moveRight = true);
    void playHitAnimation();
    void playFaintAnimation();

    QPoint spriteOffset() const       { return m_offset; }
    void   setSpriteOffset(QPoint p)  { m_offset = p; update(); }

protected:
    void paintEvent(QPaintEvent*) override;
    QSize sizeHint() const override { return {160, 160}; }

private:
    QPixmap m_pixmap;
    QColor  m_fallbackColor = QColor("#4A90D9");
    QString m_fallbackLabel;
    bool    m_mirrored      = false;
    bool    m_fainting      = false;
    float   m_opacity       = 1.0f;
    QPoint  m_offset        = {0, 0};

    QPropertyAnimation* m_moveAnim;
};

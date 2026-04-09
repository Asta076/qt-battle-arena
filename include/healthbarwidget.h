#pragma once
#include <QWidget>

class QPropertyAnimation;

class HealthBarWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(float healthPercent READ healthPercent
                   WRITE setHealthPercent)
public:
    explicit HealthBarWidget(QWidget* parent = nullptr);

    float healthPercent() const { return m_percent; }
    void  setHealthPercent(float p);

    void  animateTo(float targetPercent);

protected:
    void paintEvent(QPaintEvent*) override;
    QSize sizeHint() const override { return {200, 18}; }

private:
    float               m_percent = 1.0f;
    QPropertyAnimation* m_anim;
};

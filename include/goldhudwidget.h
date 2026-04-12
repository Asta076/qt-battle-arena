#pragma once
#include <QWidget>

class GoldHudWidget : public QWidget {
    Q_OBJECT
public:
    explicit GoldHudWidget(QWidget* parent = nullptr);

    void setGold(int gold);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    int m_gold = 0;
};
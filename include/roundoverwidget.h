#pragma once
#include <QWidget>

class QLabel;

class RoundOverWidget : public QWidget {
    Q_OBJECT
public:
    explicit RoundOverWidget(QWidget* parent = nullptr);
    void show(const QString& message);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QLabel* m_label;
};

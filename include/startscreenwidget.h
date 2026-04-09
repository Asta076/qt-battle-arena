#pragma once
#include <QWidget>
#include "gameengine.h"

class QLabel;
class QPushButton;
class QComboBox;

class StartScreenWidget : public QWidget {
    Q_OBJECT
public:
    explicit StartScreenWidget(GameEngine* engine, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    GameEngine*  m_engine;
    QPushButton* m_startBtn;
    QPushButton* m_loadBtn;
    QComboBox*   m_difficultyBox;
};

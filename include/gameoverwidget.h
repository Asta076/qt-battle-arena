#pragma once
#include <QWidget>
#include "gameengine.h"

class QLabel;

class GameOverWidget : public QWidget {
    Q_OBJECT
public:
    explicit GameOverWidget(GameEngine* engine, QWidget* parent = nullptr);

private slots:
    void onGameOver(bool playerWon, int pScore, int eScore);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QLabel* m_resultLabel;
    QLabel* m_scoreLabel;
    GameEngine* m_engine;
signals:
    void returnToOverworld();
};

#pragma once

#include <QWidget>

class QLabel;
class QTableWidget;
class GameEngine;

class GameOverWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GameOverWidget(GameEngine* engine, QWidget* parent = nullptr);

    void showDungeonResults(int coinsEarned, int wavesSurvived);

signals:
    void returnToOverworld();
    void backToMenuRequested();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QLabel* m_resultLabel = nullptr;
    QLabel* m_scoreLabel = nullptr;
    QTableWidget* m_resultsTable = nullptr;

    GameEngine* m_engine = nullptr;
};

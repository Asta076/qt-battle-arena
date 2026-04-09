#pragma once
#include <QWidget>
#include "gameengine.h"

class QTableWidget;

class ScoreboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit ScoreboardWidget(GameEngine* engine, QWidget* parent = nullptr);

private slots:
    void onStateChanged(GameState state);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void refresh();
    GameEngine*   m_engine;
    QTableWidget* m_table;
};

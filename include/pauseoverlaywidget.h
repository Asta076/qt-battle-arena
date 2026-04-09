#pragma once
#include <QWidget>
#include "gameengine.h"

class PauseOverlayWidget : public QWidget {
    Q_OBJECT
public:
    explicit PauseOverlayWidget(GameEngine* engine, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent*) override;
};

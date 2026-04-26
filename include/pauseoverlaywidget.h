#pragma once
#include <QWidget>

class PauseOverlayWidget : public QWidget {
    Q_OBJECT
public:
    // showSave = true for overworld, false for dungeon/battle
    explicit PauseOverlayWidget(bool showSave, QWidget* parent = nullptr);

signals:
    void resumeRequested();
    void saveRequested();
    void menuRequested();

protected:
    void paintEvent(QPaintEvent*) override;
};

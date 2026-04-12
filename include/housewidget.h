#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class AudioManager;

class HouseWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HouseWidget(AudioManager* audio, QWidget* parent = nullptr);

signals:
    void backToOverworld();

private:
    AudioManager* m_audio = nullptr;
    QLabel* m_label = nullptr;
    QPushButton* m_backButton = nullptr;
};

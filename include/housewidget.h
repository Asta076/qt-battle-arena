#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class AudioManager;
class PlayerProfile;
class HouseWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HouseWidget(AudioManager* audio, QWidget* parent = nullptr);
void setProfile(PlayerProfile* profile);
signals:
    void backToOverworld();

private:
    AudioManager* m_audio = nullptr;
    PlayerProfile* m_profile = nullptr;

    QLabel* m_label = nullptr;
    QPushButton* m_backButton = nullptr;

    QLabel* m_statsLabel = nullptr;
    QLabel* m_inventoryLabel = nullptr;
};

#pragma once

#include <QWidget>
#include "playerprofile.h"
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
    QString getItemName(ItemType type) const;
    AudioManager* m_audio = nullptr;
    PlayerProfile* m_profile = nullptr;

    QLabel* m_label = nullptr;
    QPushButton* m_backButton = nullptr;

    QLabel* m_statsLabel = nullptr;
    QLabel* m_inventoryLabel = nullptr;
};

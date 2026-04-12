#pragma once
#include <QWidget>
#include "playerprofile.h"

class QPushButton;
class QLabel;

enum class SlotMode { NewGame, Load };

class SaveSlotWidget : public QWidget {
    Q_OBJECT
public:
    explicit SaveSlotWidget(QWidget* parent = nullptr);

    void refresh(SlotMode mode);

    static QString slotPath(int index);   // "save_slot_0.json" etc.

signals:
    void newGameRequested(int slotIndex);
    void loadRequested(int slotIndex);
    void backRequested();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void buildCards();
    void updateCard(int index);
    void onSlotClicked(int index);

    SlotMode      m_mode = SlotMode::NewGame;
    PlayerProfile m_profiles[3];
    bool          m_occupied[3] = { false, false, false };

    struct SlotCard {
        QWidget*     widget     = nullptr;
        QLabel*      nameLabel  = nullptr;
        QLabel*      classLabel = nullptr;
        QLabel*      statsLabel = nullptr;
        QPushButton* actionBtn  = nullptr;
    } m_cards[3];
};
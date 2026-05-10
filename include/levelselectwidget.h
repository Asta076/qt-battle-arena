#pragma once
#include <QWidget>
#include "levelmanager.h"
#include "playerprofile.h"

class QPushButton;
class QLabel;

// ─────────────────────────────────────────────────────────────────────────────
//  LevelSelectWidget
//
//  Full-screen level selection screen shown when the player enters the LEVELS
//  portal in the overworld. Displays all 5 levels as cards with lock/complete
//  status. Emits levelSelected(id) when the player picks an unlocked level.
// ─────────────────────────────────────────────────────────────────────────────
class LevelSelectWidget : public QWidget {
    Q_OBJECT
public:
    explicit LevelSelectWidget(const LevelManager* manager, QWidget* parent = nullptr);

    // Call every time the screen is shown to refresh lock/complete states
    void refresh(const PlayerProfile& profile);

signals:
    void levelSelected(int levelId);   // player chose an unlocked level
    void backRequested();              // player pressed back

protected:
    void paintEvent (QPaintEvent*) override;
    void keyPressEvent(QKeyEvent*) override;

private:
    void buildCards();
    void selectCard(int index);
    void confirmSelection();

    const LevelManager* m_manager;
    const PlayerProfile* m_profile = nullptr;

    struct LevelCard {
        QWidget*     widget    = nullptr;
        QLabel*      numLabel  = nullptr;
        QLabel*      nameLabel = nullptr;
        QLabel*      statusLabel = nullptr;
        QPushButton* btn       = nullptr;
        int          levelId   = 0;
    };

    static constexpr int LEVEL_COUNT = 5;
    LevelCard    m_cards[LEVEL_COUNT];
    int          m_cursor = 0;

    QPushButton* m_backBtn = nullptr;
    QLabel*      m_hintLabel = nullptr;
};

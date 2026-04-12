#pragma once
#include <QWidget>
#include "playerprofile.h"

class QLabel;
class QKeyEvent;

// ─────────────────────────────────────────────────────────────────────────────
//  ItemMenuWidget
//
//  The overlay that appears when the player presses ITEM during battle.
//  Shows the player's current inventory in a 2×3 grid.
//  Emits itemChosen(ItemType) when the player confirms a selection.
//  Emits cancelled() when the player backs out with Escape.
//
//  This widget only READS the inventory — it never modifies it.
//  MainWindow is responsible for deducting the item after use.
// ─────────────────────────────────────────────────────────────────────────────
class ItemMenuWidget : public QWidget {
    Q_OBJECT

public:
    explicit ItemMenuWidget(QWidget* parent = nullptr);

    // Call this every time the menu is shown so it reflects current inventory
    void refresh(const PlayerProfile* profile, int itemsUsedThisBattle, int maxItemsPerBattle);

signals:
    void itemChosen(ItemType type);   // player confirmed a selection
    void cancelled();                 // player pressed Escape

protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    struct SlotInfo {
        ItemType type;
        QString  name;
        QString  description;
        int      quantity   = 0;
        bool     isEmpty    = true;
    };

    void buildSlots(const PlayerProfile* profile);
    void moveCursor(int delta);
    void confirmSelection();

    // Layout constants
    static constexpr int COLS            = 2;
    static constexpr int ROWS            = 3;
    static constexpr int TOTAL_SLOTS     = COLS * ROWS;
    static constexpr int SLOT_W          = 200;
    static constexpr int SLOT_H          = 52;
    static constexpr int SLOT_GAP        = 8;
    static constexpr int PADDING         = 16;

    SlotInfo m_slots[TOTAL_SLOTS];
    int      m_cursorIndex      = 0;
    int      m_usesLeft         = 0;   // how many items can still be used this battle
};
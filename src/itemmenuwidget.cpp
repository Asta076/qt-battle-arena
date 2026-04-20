#include "itemmenuwidget.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
// ── Item display metadata — add a new row here when you add a new ItemType ──
struct ItemMeta {
    ItemType type;
    QString  name;
    QString  description;
};

static const ItemMeta ITEM_META[] = {
    { ItemType::HealthPotion, "HEALTH POT",  "Restore 35% HP"    },
    { ItemType::SpPotion,     "SP POTION",   "Restore 50 SP"     },
    { ItemType::AttackBoost,  "ATK BOOST",   "+50% ATK 1 turn"   },
    { ItemType::DefenseBoost, "DEF SHIELD",  "Block 30% next hit"},
    };
static constexpr int META_COUNT = sizeof(ITEM_META) / sizeof(ITEM_META[0]);

ItemMenuWidget::ItemMenuWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::StrongFocus);

    // Size: 2 columns of slots + padding on all sides
    int totalW = COLS * SLOT_W + (COLS - 1) * SLOT_GAP + PADDING * 2;
    int totalH = ROWS * SLOT_H + (ROWS - 1) * SLOT_GAP + PADDING * 2
                 + 40;   // +40 for the "uses left" label at top
    setFixedSize(totalW, totalH);
}

void ItemMenuWidget::refresh(const PlayerProfile* profile,
                             int itemsUsedThisBattle,
                             int maxItemsPerBattle)
{
    m_usesLeft = maxItemsPerBattle - itemsUsedThisBattle;
    buildSlots(profile);

    // Move cursor to first available slot
    m_cursorIndex = 0;
    for (int i = 0; i < TOTAL_SLOTS; ++i) {
        if (!m_slots[i].isEmpty && m_slots[i].quantity > 0) {
            m_cursorIndex = i;
            break;
        }
    }
    update();
}

void ItemMenuWidget::buildSlots(const PlayerProfile* profile)
{
    // Clear all slots first
    for (int i = 0; i < TOTAL_SLOTS; ++i)
        m_slots[i] = SlotInfo{};

    // Fill slots from inventory — one slot per item type that exists
    int slot = 0;
    for (int m = 0; m < META_COUNT && slot < TOTAL_SLOTS; ++m) {
        int qty = profile->itemCount(ITEM_META[m].type);
        if (qty > 0) {
            m_slots[slot].type        = ITEM_META[m].type;
            m_slots[slot].name        = ITEM_META[m].name;
            m_slots[slot].description = ITEM_META[m].description;
            m_slots[slot].quantity    = qty;
            m_slots[slot].isEmpty     = false;
            ++slot;
        }
    }
    // Remaining slots stay empty (isEmpty = true)
}

void ItemMenuWidget::moveCursor(int delta)
{
    // Try to find next non-empty slot in the given direction
    int next = m_cursorIndex + delta;
    while (next >= 0 && next < TOTAL_SLOTS) {
        if (!m_slots[next].isEmpty)  {
            m_cursorIndex = next;
            update();
            return;
        }
        next += delta;
    }
    // If nothing found in that direction, don't move
}

void ItemMenuWidget::confirmSelection()
{
    if (m_usesLeft <= 0) return;
    const SlotInfo& slot = m_slots[m_cursorIndex];
    if (slot.isEmpty || slot.quantity <= 0) return;
    emit itemChosen(slot.type);
}

void ItemMenuWidget::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Up:    moveCursor(-COLS); break;
    case Qt::Key_Down:  moveCursor(+COLS); break;
    case Qt::Key_Left:  moveCursor(-1);    break;
    case Qt::Key_Right: moveCursor(+1);    break;

    case Qt::Key_Return:
    case Qt::Key_Space:
        confirmSelection();
        break;

    case Qt::Key_Escape:
    case Qt::Key_B:
        emit cancelled();
        break;

    default:
        QWidget::keyPressEvent(event);
    }
}

void ItemMenuWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    const int totalW = width();
    const int totalH = height();

    // ── Overall background ────────────────────────────────────────────────────
    p.setPen(QPen(QColor("#F0E8D0"), 3));
    p.setBrush(QColor(13, 13, 26, 230));
    p.drawRect(0, 0, totalW, totalH);

    // ── "Uses remaining" label ────────────────────────────────────────────────
    p.setPen(m_usesLeft > 0 ? QColor("#FFE066") : QColor("#EE3333"));
    p.setFont(QFont("Press Start 2P", 7));
    p.drawText(QRect(0, 6, totalW, 28),
               Qt::AlignCenter,
               m_usesLeft > 0
                   ? QString("ITEMS LEFT: %1").arg(m_usesLeft)
                   : "NO USES LEFT THIS BATTLE");

    // ── Slots ─────────────────────────────────────────────────────────────────
    const int startY = 40;   // below the label

    for (int i = 0; i < TOTAL_SLOTS; ++i) {
        const int col  = i % COLS;
        const int row  = i / COLS;
        const int x    = PADDING + col * (SLOT_W + SLOT_GAP);
        const int y    = startY + row * (SLOT_H + SLOT_GAP);
        const bool sel = (i == m_cursorIndex);
        const SlotInfo& slot = m_slots[i];

        // Slot background
        if (slot.isEmpty) {
            p.setBrush(QColor(30, 30, 50, 180));
            p.setPen(QPen(QColor("#3A3A5A"), 2));
        } else if (sel && m_usesLeft > 0) {
            p.setBrush(QColor("#1A2A4A"));
            p.setPen(QPen(QColor("#FFE066"), 2));
        } else {
            p.setBrush(QColor("#16213E"));
            p.setPen(QPen(QColor("#F0E8D0"), 1));
        }
        p.drawRect(x, y, SLOT_W, SLOT_H);

        if (slot.isEmpty) continue;

        // Cursor arrow
        if (sel && m_usesLeft > 0) {
            p.setPen(QColor("#FFE066"));
            p.setFont(QFont("Press Start 2P", 7));
            p.drawText(x + 4, y + 18, "►");
        }

        // Item name
        p.setPen(slot.quantity > 0 && m_usesLeft > 0
                     ? QColor("#F0E8D0") : QColor("#888880"));
        p.setFont(QFont("Press Start 2P", 7));
        p.drawText(x + 18, y + 18, slot.name);

        // Description
        p.setPen(QColor("#A0A0C0"));
        p.setFont(QFont("Press Start 2P", 6));
        p.drawText(x + 18, y + 34, slot.description);

        // Quantity badge — top-right corner of slot
        QString qtyText = QString("x%1").arg(slot.quantity);
        p.setPen(QColor("#FFD700"));
        p.setFont(QFont("Press Start 2P", 7));
        QRect qtyRect(x + SLOT_W - 36, y + 4, 32, 16);
        p.drawText(qtyRect, Qt::AlignRight | Qt::AlignVCenter, qtyText);
    }
}
void ItemMenuWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;

    const int startY = 40;
    for (int i = 0; i < TOTAL_SLOTS; ++i) {
        const int col = i % COLS;
        const int row = i / COLS;
        const int x   = PADDING + col * (SLOT_W + SLOT_GAP);
        const int y   = startY  + row * (SLOT_H + SLOT_GAP);

        QRect slotRect(x, y, SLOT_W, SLOT_H);
        if (slotRect.contains(event->pos())) {
            if (m_slots[i].isEmpty) return;
            m_cursorIndex = i;   // move cursor to clicked slot
            update();
            confirmSelection();  // attempt to use it
            return;
        }
    }
}
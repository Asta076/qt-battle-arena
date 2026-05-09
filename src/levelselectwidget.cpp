#include "levelselectwidget.h"
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QKeyEvent>
#include <QRandomGenerator>
#include <QFont>

// ─────────────────────────────────────────────────────────────────────────────
//  Mini scene preview generator
//  Draws a small hand-crafted scene for each level theme into a QPixmap
// ─────────────────────────────────────────────────────────────────────────────

static QPixmap generatePreview(LevelTheme theme, bool locked, int W, int H)
{
    QPixmap px(W, H);
    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing, false);

    // ── Base background per theme ─────────────────────────────────────────────
    switch (theme) {
    case LevelTheme::Cave:    p.fillRect(0,0,W,H, QColor("#1a2e1a")); break;
    case LevelTheme::Forest:  p.fillRect(0,0,W,H, QColor("#0d0014")); break;
    case LevelTheme::Peak:    p.fillRect(0,0,W,H, QColor("#001929")); break;
    case LevelTheme::Volcano: p.fillRect(0,0,W,H, QColor("#1a0400")); break;
    case LevelTheme::Arena:   p.fillRect(0,0,W,H, QColor("#080808")); break;
    }

    if (locked) {
        // Greyed out with lock icon
        p.fillRect(0, 0, W, H, QColor(0, 0, 0, 160));
        p.setPen(QColor("#555570"));
        p.setFont(QFont("Arial", 20));
        p.drawText(QRect(0, 0, W, H), Qt::AlignCenter, "🔒");
        return px;
    }

    switch (theme) {

    // ── Level 1 — Goblin Cave ─────────────────────────────────────────────────
    case LevelTheme::Cave: {
        // Grass strips
        p.fillRect(0, H*2/3, W, H/3, QColor("#2e7d32"));
        p.fillRect(0, H*3/4, W, H/4, QColor("#388e3c"));

        // Dirt path
        p.fillRect(W*2/5, 0, W/5, H, QColor("#5d4037"));

        // Stalactites hanging from top
        p.setBrush(QColor("#37474f"));
        p.setPen(Qt::NoPen);
        for (int x = 8; x < W - 8; x += 18) {
            int h = 10 + (x % 3) * 5;
            QPoint pts[3] = { {x, 0}, {x+8, 0}, {x+4, h} };
            p.drawPolygon(pts, 3);
        }

        // Rock debris
        p.setBrush(QColor("#455a64"));
        p.drawRect(12, H*2/3 - 6, 14, 8);
        p.drawRect(W-30, H*2/3 - 4, 18, 6);
        p.drawRect(W/3, H*5/6, 10, 6);

        // Small torch glow
        p.setBrush(QColor(255, 160, 0, 100));
        p.drawEllipse(W-18, H/3, 12, 12);
        p.setBrush(QColor("#FFD700"));
        p.drawEllipse(W-14, H/3+4, 4, 4);
        break;
    }

    // ── Level 2 — Haunted Forest ──────────────────────────────────────────────
    case LevelTheme::Forest: {
        // Dark grass
        p.fillRect(0, H*3/4, W, H/4, QColor("#1a1a2e"));

        // Purple overlay
        p.fillRect(0, 0, W, H, QColor(40, 0, 60, 120));

        // Dead tree trunks
        p.setBrush(QColor("#1c2a1c"));
        p.setPen(Qt::NoPen);
        const int trunks[][3] = {{10,H/4,8},{W/3,H/6,6},{W*2/3,H/5,7},{W-20,H/4,8}};
        for (auto& t : trunks) {
            p.drawRect(t[0], t[1], t[2], H - t[1]);
            // Bare branch left
            p.drawRect(t[0]-10, t[1]+12, 10, 3);
            // Bare branch right
            p.drawRect(t[0]+t[2], t[1]+20, 10, 3);
        }

        // Skull symbols
        p.setPen(QColor(150, 80, 180, 180));
        p.setFont(QFont("Arial", 8));
        p.drawText(W/4,   H/3, "☠");
        p.drawText(W*3/4, H/2, "☠");

        // Eerie ground mist
        p.fillRect(0, H*3/4, W, 8, QColor(80, 0, 120, 60));
        break;
    }

    // ── Level 3 — Frozen Peak ─────────────────────────────────────────────────
    case LevelTheme::Peak: {
        // Snow ground
        p.fillRect(0, H*2/3, W, H/3, QColor("#b0bec5"));
        p.fillRect(0, H*3/4, W, H/4, QColor("#cfd8dc"));

        // Blue sky overlay
        p.fillRect(0, 0, W, H*2/3, QColor(0, 60, 120, 80));

        // Ice shards from ground
        p.setBrush(QColor(140, 210, 255, 200));
        p.setPen(Qt::NoPen);
        const int shards[][2] = {{15,H*2/3},{45,H*2/3},{W/2,H*2/3},{W-40,H*2/3},{W-15,H*2/3}};
        for (auto& s : shards) {
            int h = 20 + (s[0] % 4) * 8;
            QPoint pts[3] = {{s[0], s[1]}, {s[0]+10, s[1]}, {s[0]+5, s[1]-h}};
            p.drawPolygon(pts, 3);
        }

        // Snowflake dots
        p.setBrush(QColor(200, 230, 255, 200));
        for (int i = 0; i < 12; i++) {
            int x = (i * 37) % W;
            int y = (i * 23) % (H * 2/3);
            int r = 1 + (i % 3);
            p.drawEllipse(x, y, r*2, r*2);
        }

        // Mountain silhouette
        p.setBrush(QColor(80, 100, 120, 120));
        QPoint mtn[3] = {{W/2-30, H*2/3}, {W/2, H/8}, {W/2+30, H*2/3}};
        p.drawPolygon(mtn, 3);
        break;
    }

    // ── Level 4 — Volcanic Dungeon ────────────────────────────────────────────
    case LevelTheme::Volcano: {
        // Dark rock floor
        p.fillRect(0, H*2/3, W, H/3, QColor("#1a0000"));
        p.fillRect(0, H*3/4, W, H/4, QColor("#260000"));

        // Red sky glow
        p.fillRect(0, 0, W, H*2/3, QColor(140, 20, 0, 80));

        // Lava pools on ground
        const int pools[][3] = {{8,H*3/4,30},{W/2-15,H*3/4+4,24},{W-40,H*3/4,28}};
        for (auto& pool : pools) {
            // Glow
            p.setBrush(QColor(255, 80, 0, 60));
            p.setPen(Qt::NoPen);
            p.drawEllipse(pool[0]-4, pool[1]-4, pool[2]+8, 14);
            // Lava
            p.setBrush(QColor(255, 140, 0, 200));
            p.drawEllipse(pool[0], pool[1], pool[2], 10);
        }

        // Dark volcanic rocks
        p.setBrush(QColor("#1a0000"));
        p.drawRect(W/3, H*2/3-8, 20, 10);
        p.drawRect(W*2/3-10, H*2/3-6, 16, 8);

        // Volcano silhouette in background
        p.setBrush(QColor(80, 10, 0, 160));
        QPoint vol[3] = {{W/2-40, H*2/3}, {W/2, H/6}, {W/2+40, H*2/3}};
        p.drawPolygon(vol, 3);

        // Eruption glow
        p.setBrush(QColor(255, 100, 0, 100));
        p.drawEllipse(W/2-10, H/6-8, 20, 12);
        break;
    }

    // ── Level 5 — The Final Arena ─────────────────────────────────────────────
    case LevelTheme::Arena: {
        // Stone floor
        p.fillRect(0, H*2/3, W, H/3, QColor("#1a1a1a"));

        // Dark overlay
        p.fillRect(0, 0, W, H, QColor(0, 0, 0, 100));

        // Arena boundary ring
        const int margin = 6, thick = 3;
        p.setPen(QPen(QColor("#B8860B"), thick));
        p.setBrush(Qt::NoBrush);
        p.drawRect(margin, margin, W-margin*2, H-margin*2);

        // Stone tile lines on floor
        p.setPen(QPen(QColor("#333333"), 1));
        for (int x = 0; x < W; x += 16)
            p.drawLine(x, H*2/3, x, H);
        p.drawLine(0, H*2/3, W, H*2/3);

        // Corner torches
        p.setPen(Qt::NoPen);
        const int corners[][2] = {{margin, margin}, {W-margin-8, margin},
                                   {margin, H-margin-8}, {W-margin-8, H-margin-8}};
        for (auto& c : corners) {
            p.setBrush(QColor(255, 160, 0, 80));
            p.drawEllipse(c[0]-3, c[1]-3, 14, 14);
            p.setBrush(QColor("#FFD700"));
            p.drawEllipse(c[0]+1, c[1]+1, 6, 6);
        }

        // Center arena marking
        p.setPen(QPen(QColor("#B8860B"), 1));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(W/2-20, H/2-12, 40, 24);
        break;
    }
    }

    return px;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Theme accent colour helper
// ─────────────────────────────────────────────────────────────────────────────

static QColor themeAccent(LevelTheme t)
{
    switch (t) {
    case LevelTheme::Cave:    return QColor("#A5D6A7");
    case LevelTheme::Forest:  return QColor("#CE93D8");
    case LevelTheme::Peak:    return QColor("#80D8FF");
    case LevelTheme::Volcano: return QColor("#FF6E40");
    case LevelTheme::Arena:   return QColor("#FFD700");
    }
    return QColor("#FFFFFF");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────

LevelSelectWidget::LevelSelectWidget(const LevelManager* manager, QWidget* parent)
    : QWidget(parent)
    , m_manager(manager)
{
    setFocusPolicy(Qt::StrongFocus);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(40, 30, 40, 30);
    root->setSpacing(0);

    // ── Title ─────────────────────────────────────────────────────────────────
    auto* title = new QLabel("— SELECT YOUR LEVEL —", this);
    title->setAlignment(Qt::AlignCenter);
    title->setFont(QFont("Press Start 2P", 10));
    title->setStyleSheet("color: #FFE066;");
    root->addWidget(title);
    root->addSpacing(24);

    // ── Hint label ────────────────────────────────────────────────────────────
    m_hintLabel = new QLabel("", this);
    m_hintLabel->setAlignment(Qt::AlignCenter);
    m_hintLabel->setFont(QFont("Press Start 2P", 6));
    m_hintLabel->setStyleSheet("color: #888880;");
    m_hintLabel->setWordWrap(true);

    // ── Cards row ─────────────────────────────────────────────────────────────
    auto* cardsRow = new QHBoxLayout;
    cardsRow->setSpacing(16);
    cardsRow->setAlignment(Qt::AlignCenter);

    const LevelDef* defs[5];
    for (int i = 0; i < 5; ++i) defs[i] = m_manager->level(i + 1);

    for (int i = 0; i < LEVEL_COUNT; ++i) {
        const LevelDef* lvl = defs[i];

        auto* card = new QWidget(this);
        card->setObjectName("characterCard");
        card->setFixedSize(180, 280);

        auto* layout = new QVBoxLayout(card);
        layout->setContentsMargins(10, 10, 10, 10);
        layout->setSpacing(6);

        // Preview image placeholder (filled in refresh())
        auto* preview = new QLabel(card);
        preview->setFixedSize(158, 100);
        preview->setAlignment(Qt::AlignCenter);
        preview->setObjectName("levelPreview");

        auto* numLabel  = new QLabel(QString("LEVEL %1").arg(i + 1), card);
        auto* nameLabel = new QLabel(lvl ? lvl->name : "???", card);
        auto* statusLabel = new QLabel("🔒 LOCKED", card);
        auto* btn = new QPushButton("SELECT", card);

        numLabel->setAlignment(Qt::AlignCenter);
        nameLabel->setAlignment(Qt::AlignCenter);
        statusLabel->setAlignment(Qt::AlignCenter);

        numLabel->setFont(QFont("Press Start 2P", 6));
        nameLabel->setFont(QFont("Press Start 2P", 6));
        statusLabel->setFont(QFont("Press Start 2P", 6));

        QColor accent = lvl ? themeAccent(lvl->theme) : QColor("#FFFFFF");
        numLabel->setStyleSheet(QString("color: %1;").arg(accent.name()));
        nameLabel->setStyleSheet("color: #F0E8D0;");

        btn->setFixedWidth(140);

        const int captured = i;
        connect(btn, &QPushButton::clicked, this, [this, captured] {
            selectCard(captured);
            confirmSelection();
        });

        layout->addWidget(preview,     0, Qt::AlignCenter);
        layout->addWidget(numLabel,    0, Qt::AlignCenter);
        layout->addWidget(nameLabel,   0, Qt::AlignCenter);
        layout->addWidget(statusLabel, 0, Qt::AlignCenter);
        layout->addStretch();
        layout->addWidget(btn,         0, Qt::AlignCenter);

        m_cards[i] = { card, numLabel, nameLabel, statusLabel, btn, i + 1 };
        cardsRow->addWidget(card);
    }

    root->addLayout(cardsRow);
    root->addSpacing(12);
    root->addWidget(m_hintLabel);
    root->addSpacing(16);

    // ── Back button ───────────────────────────────────────────────────────────
    m_backBtn = new QPushButton("← BACK TO OVERWORLD", this);
    m_backBtn->setFixedWidth(280);
    connect(m_backBtn, &QPushButton::clicked, this, &LevelSelectWidget::backRequested);
    root->addWidget(m_backBtn, 0, Qt::AlignCenter);

    root->addStretch();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Refresh — updates lock/complete state and preview images
// ─────────────────────────────────────────────────────────────────────────────

void LevelSelectWidget::refresh(const PlayerProfile& profile)
{
    m_profile = &profile;

    for (int i = 0; i < LEVEL_COUNT; ++i) {
        LevelCard& c   = m_cards[i];
        const LevelDef* lvl = m_manager->level(c.levelId);
        if (!lvl) continue;

        bool unlocked  = m_manager->isUnlocked(c.levelId, profile);
        bool completed = m_manager->isCompleted(c.levelId, profile);

        // ── Preview image ─────────────────────────────────────────────────────
        QLabel* preview = c.widget->findChild<QLabel*>("levelPreview");
        if (preview) {
            QPixmap px = generatePreview(lvl->theme, !unlocked, 158, 100);
            preview->setPixmap(px);
        }

        // ── Status label ──────────────────────────────────────────────────────
        if (!unlocked) {
            c.statusLabel->setText("🔒  " + (lvl->unlockHint.isEmpty()
                                   ? "LOCKED" : lvl->unlockHint));
            c.statusLabel->setStyleSheet("color: #555570;");
            c.btn->setEnabled(false);
            c.btn->setText("LOCKED");
        } else if (completed) {
            c.statusLabel->setText("✓  COMPLETE");
            c.statusLabel->setStyleSheet("color: #A5D6A7;");
            c.btn->setEnabled(true);
            c.btn->setText("REPLAY");
        } else {
            c.statusLabel->setText("► AVAILABLE");
            c.statusLabel->setStyleSheet(
                QString("color: %1;").arg(themeAccent(lvl->theme).name()));
            c.btn->setEnabled(true);
            c.btn->setText("SELECT");
        }
    }

    selectCard(m_cursor);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Selection
// ─────────────────────────────────────────────────────────────────────────────

void LevelSelectWidget::selectCard(int index)
{
    m_cursor = index;

    const LevelDef* lvl = m_manager->level(index + 1);
    bool unlocked = (m_profile && lvl)
                    ? m_manager->isUnlocked(index + 1, *m_profile)
                    : false;

    if (!unlocked && lvl && !lvl->unlockHint.isEmpty())
        m_hintLabel->setText("Requires: " + lvl->unlockHint);
    else
        m_hintLabel->setText("");
}

void LevelSelectWidget::confirmSelection()
{
    const LevelDef* lvl = m_manager->level(m_cursor + 1);
    if (!lvl || !m_profile) return;
    if (!m_manager->isUnlocked(m_cursor + 1, *m_profile)) return;
    emit levelSelected(m_cursor + 1);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Paint — dark background with gold accent
// ─────────────────────────────────────────────────────────────────────────────

void LevelSelectWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), QColor("#0D0D1A"));

    QPixmap bg(":/backgrounds/battle_bg.png");
    if (!bg.isNull()) {
        p.setOpacity(0.25);
        p.drawPixmap(rect(), bg);
        p.setOpacity(1.0);
    }

    p.setPen(QPen(QColor("#FFE066"), 1));
    p.drawLine(0, 0, width(), 0);
    p.drawLine(0, height()-1, width(), height()-1);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Keyboard navigation
// ─────────────────────────────────────────────────────────────────────────────

void LevelSelectWidget::keyPressEvent(QKeyEvent* e)
{
    switch (e->key()) {
    case Qt::Key_Left:
        selectCard(qMax(0, m_cursor - 1));
        break;
    case Qt::Key_Right:
        selectCard(qMin(LEVEL_COUNT - 1, m_cursor + 1));
        break;
    case Qt::Key_Return:
    case Qt::Key_Space:
        confirmSelection();
        break;
    case Qt::Key_Escape:
        emit backRequested();
        break;
    default:
        QWidget::keyPressEvent(e);
    }
}

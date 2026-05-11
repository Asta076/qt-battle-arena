#include "levelbattlewidget.h"
#include "healthbarwidget.h"
#include "spritewidget.h"
#include "pauseoverlaywidget.h"
#include "itemmenuwidget.h"
#include "spritecache.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QKeyEvent>
#include <QTimer>
#include <QFont>

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────

LevelBattleWidget::LevelBattleWidget(GameEngine* engine,
                                     AudioManager* audio,
                                     PlayerProfile* profile,
                                     QWidget* parent)
    : QWidget(parent)
    , m_engine(engine)
    , m_audio(audio)
    , m_profile(profile)
{
    setFocusPolicy(Qt::StrongFocus);

    // ── Header labels ─────────────────────────────────────────────────────────
    m_headerLabel = new QLabel(this);
    m_headerLabel->setAlignment(Qt::AlignCenter);
    m_headerLabel->setFont(QFont("Press Start 2P", 9));
    m_headerLabel->setStyleSheet("color: #FFE066;");

    m_subheaderLabel = new QLabel(this);
    m_subheaderLabel->setAlignment(Qt::AlignCenter);
    m_subheaderLabel->setFont(QFont("Press Start 2P", 7));
    m_subheaderLabel->setStyleSheet("color: #A0A0C0;");

    // ── Enemy side ────────────────────────────────────────────────────────────
    m_enemyNameLabel = new QLabel(this);
    m_enemyNameLabel->setAlignment(Qt::AlignCenter);
    m_enemyNameLabel->setFont(QFont("Press Start 2P", 8));
    m_enemyNameLabel->setStyleSheet("color: #F0E8D0;");

    m_enemySprite = new SpriteWidget(this);
    m_enemySprite->setFixedSize(200, 200);
    m_enemySprite->setMirrored(true);

    m_enemyHP = new HealthBarWidget(this);
    m_enemyHP->setFixedSize(220, 16);
    m_enemySP = new HealthBarWidget(this);
    m_enemySP->setFixedSize(220, 10);
    m_enemySP->setFixedBarColor(QColor("#4A90D9"));
    m_enemySP->animateTo(0.0f);

    // ── Player side ───────────────────────────────────────────────────────────
    m_playerNameLabel = new QLabel(this);
    m_playerNameLabel->setAlignment(Qt::AlignCenter);
    m_playerNameLabel->setFont(QFont("Press Start 2P", 8));
    m_playerNameLabel->setStyleSheet("color: #F0E8D0;");

    m_playerSprite = new SpriteWidget(this);
    m_playerSprite->setFixedSize(200, 200);

    m_playerHP = new HealthBarWidget(this);
    m_playerHP->setFixedSize(220, 16);
    m_playerSP = new HealthBarWidget(this);
    m_playerSP->setFixedSize(220, 10);
    m_playerSP->setFixedBarColor(QColor("#4A90D9"));
    m_playerSP->animateTo(0.0f);

    // ── Score (boss multi-round) ───────────────────────────────────────────────
    m_scoreLabel = new QLabel("", this);
    m_scoreLabel->setAlignment(Qt::AlignCenter);
    m_scoreLabel->setFont(QFont("Press Start 2P", 7));
    m_scoreLabel->setStyleSheet("color: #A0A0C0;");

    // ── Battle message ────────────────────────────────────────────────────────
    m_messageLabel = new QLabel("", this);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setFont(QFont("Press Start 2P", 7));
    m_messageLabel->setStyleSheet("color: #F0E8D0;");

    // ── Action buttons ────────────────────────────────────────────────────────
    m_fightBtn   = new QPushButton("► FIGHT",   this);
    m_specialBtn = new QPushButton("  SPECIAL", this);
    m_itemBtn    = new QPushButton("  ITEM",    this);
    m_runBtn     = new QPushButton("  RUN",     this);

    for (auto* btn : {m_fightBtn, m_specialBtn, m_itemBtn, m_runBtn})
        btn->setFixedWidth(160);

    connect(m_fightBtn,   &QPushButton::clicked, m_engine, &GameEngine::onPlayerChoseFight);
    connect(m_specialBtn, &QPushButton::clicked, m_engine, &GameEngine::onPlayerChoseSpecial);
    connect(m_runBtn,     &QPushButton::clicked, m_engine, &GameEngine::onPlayerChoseRun);
    connect(m_itemBtn,    &QPushButton::clicked, this, [this] {
        if (!m_engine->itemAvailable()) return;
        m_itemMenu->refresh(m_profile,
                            m_engine->itemsUsedThisBattle(),
                            m_engine->maxItemsPerBattle());
        setButtonsEnabled(false);
        m_itemMenu->show();
        m_itemMenu->raise();
        m_itemMenu->setFocus();
    });

    // ── Overlays ──────────────────────────────────────────────────────────────
    m_pauseOverlay = new PauseOverlayWidget(false, this);
    connect(m_pauseOverlay, &PauseOverlayWidget::resumeRequested, this, [this]{
        m_engine->onPauseToggle();
    });

    m_itemMenu = new ItemMenuWidget(this);
    m_itemMenu->hide();
    connect(m_itemMenu, &ItemMenuWidget::itemChosen,    this, &LevelBattleWidget::onItemMenuChosen);
    connect(m_itemMenu, &ItemMenuWidget::cancelled,     this, &LevelBattleWidget::onItemMenuCancelled);

    // ── Engine signals ────────────────────────────────────────────────────────
    connect(engine, &GameEngine::stateChanged,
            this,   &LevelBattleWidget::onStateChanged);
    connect(engine, &GameEngine::healthUpdated,
            this,   &LevelBattleWidget::onHealthUpdated);
    connect(engine, &GameEngine::battleActionResolved,
            this,   &LevelBattleWidget::onBattleActionResolved);
    connect(engine, &GameEngine::roundEnded,
            this,   &LevelBattleWidget::onRoundEnded);
    connect(engine, &GameEngine::energyUpdated,
            this,   &LevelBattleWidget::onEnergyUpdated);
    connect(engine, &GameEngine::battleLogMessage,
            this,   &LevelBattleWidget::showMessage);

    // Update sprites when battle starts
    connect(engine, &GameEngine::stateChanged, this, [this](GameState s) {
        if (s != GameState::PlayerTurn) return;
        const auto& chars = m_engine->getAllCharacters();
        if (chars.size() < 2) return;

        auto spritePath = [](CharacterType t, bool back) -> QString {
            switch (t) {
            case CharacterType::Warrior:
                return back ? ":/sprites/warrior_back.png" : ":/sprites/warrior_front.png";
            case CharacterType::Mage:
                return back ? ":/sprites/mage_back.png" : ":/sprites/mage_front.png";
            case CharacterType::Archer:
                return back ? ":/sprites/archer_back.png" : ":/sprites/archer_front.png";
            }
            return {};
        };

        m_playerNameLabel->setText(chars[0]->getName());
        m_enemyNameLabel->setText(chars[1]->getName());
        m_playerSprite->setSprite(spritePath(chars[0]->getType(), true));
        m_enemySprite->setSprite(spritePath(chars[1]->getType(), false));
        m_playerSprite->setFallbackColor(QColor("#4A6FA5"), chars[0]->getName());
        m_enemySprite->setFallbackColor(QColor("#A54A4A"), chars[1]->getName());
    });

    updateCursor();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Context
// ─────────────────────────────────────────────────────────────────────────────

void LevelBattleWidget::setContext(const QString& levelName,
                                   bool isBoss,
                                   LevelTheme theme)
{
    m_levelName = levelName;
    m_isBoss    = isBoss;
    m_theme     = theme;

    m_headerLabel->setText(levelName.toUpper());
    m_subheaderLabel->setText(isBoss ? "⚔  BOSS BATTLE" : "— ENCOUNTER —");
    m_scoreLabel->setVisible(isBoss);

    // Tint the accent colour to match theme
    m_headerLabel->setStyleSheet(
        QString("color: %1;").arg(themeColor().name()));

    update();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Theme helpers
// ─────────────────────────────────────────────────────────────────────────────

QColor LevelBattleWidget::themeColor() const
{
    switch (m_theme) {
    case LevelTheme::Forest:  return QColor("#CE93D8");   // purple
    case LevelTheme::Peak:    return QColor("#80D8FF");   // ice blue
    case LevelTheme::Volcano: return QColor("#FF6E40");   // lava orange
    case LevelTheme::Arena:   return QColor("#FFD700");   // gold
    default:                  return QColor("#A5D6A7");   // cave green
    }
}

QColor LevelBattleWidget::themeBgColor() const
{
    switch (m_theme) {
    case LevelTheme::Forest:  return QColor("#0D001A");
    case LevelTheme::Peak:    return QColor("#001929");
    case LevelTheme::Volcano: return QColor("#1A0400");
    case LevelTheme::Arena:   return QColor("#0A0A0A");
    default:                  return QColor("#0A1A0A");
    }
}

QColor LevelBattleWidget::themeOverlay() const
{
    switch (m_theme) {
    case LevelTheme::Forest:  return QColor(50, 0, 80, 160);
    case LevelTheme::Peak:    return QColor(0, 60, 120, 140);
    case LevelTheme::Volcano: return QColor(140, 20, 0, 140);
    case LevelTheme::Arena:   return QColor(10, 10, 10, 180);
    default:                  return QColor(0, 40, 10, 140);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Engine slots
// ─────────────────────────────────────────────────────────────────────────────

void LevelBattleWidget::onStateChanged(GameState state)
{
    bool playerTurn = (state == GameState::PlayerTurn);
    setButtonsEnabled(playerTurn);
    if (playerTurn) {
        setFocus();
        m_specialBtn->setEnabled(m_engine->playerCanUseSpecial());
        m_itemBtn->setEnabled(m_engine->itemAvailable());

        if (m_isBoss) {
            m_scoreLabel->setText(
                QString("%1 — %2")
                    .arg(m_engine->getPlayerScore())
                    .arg(m_engine->getEnemyScore()));
        }
    }
    m_pauseOverlay->setVisible(state == GameState::Paused);
}

void LevelBattleWidget::onHealthUpdated(float playerPct, float enemyPct)
{
    m_playerHP->animateTo(playerPct);
    m_enemyHP->animateTo(enemyPct);
}

void LevelBattleWidget::onEnergyUpdated(float playerPct, float enemyPct)
{
    m_playerSP->animateTo(playerPct);
    m_enemySP->animateTo(enemyPct);
    m_specialBtn->setEnabled(m_engine->playerCanUseSpecial());
}

void LevelBattleWidget::onBattleActionResolved(const BattleResult& result)
{
    bool playerAttacked = !m_engine->getAllCharacters().isEmpty() &&
        result.attackerName == m_engine->getAllCharacters()[0]->getName();

    if (result.usedSpecial)
        m_audio->playSfx("/sfx/special.wav");
    else
        m_audio->playSfx("/sfx/hit.wav");

    if (playerAttacked) {
        m_playerSprite->playAttackAnimation(true);
        QTimer::singleShot(180, this, [this, result]{
            m_enemySprite->playHitAnimation();
            if (result.defenderFainted) {
                m_enemySprite->playFaintAnimation();
                m_audio->playSfx("/sfx/faint.wav");
            }
        });
    } else {
        m_enemySprite->playAttackAnimation(false);
        QTimer::singleShot(180, this, [this, result]{
            m_playerSprite->playHitAnimation();
            if (result.defenderFainted) {
                m_playerSprite->playFaintAnimation();
                m_audio->playSfx("/sfx/faint.wav");
            }
        });
    }
}

void LevelBattleWidget::onRoundEnded(int pScore, int eScore, bool playerWon)
{
    m_scoreLabel->setText(QString("%1 — %2").arg(pScore).arg(eScore));
    showMessage(playerWon ? "✦ Round won!" : "✦ Round lost.");
}

void LevelBattleWidget::showMessage(const QString& msg)
{
    m_messageLabel->setText(msg);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Item menu
// ─────────────────────────────────────────────────────────────────────────────

void LevelBattleWidget::onItemMenuChosen(ItemType type)
{
    m_itemMenu->hide();
    setButtonsEnabled(false);
    emit itemChosen(type);
}

void LevelBattleWidget::onItemMenuCancelled()
{
    m_itemMenu->hide();
    setButtonsEnabled(true);
    setFocus();
}

void LevelBattleWidget::setButtonsEnabled(bool on)
{
    m_fightBtn->setEnabled(on);
    m_specialBtn->setEnabled(on && m_engine->playerCanUseSpecial());
    m_itemBtn->setEnabled(on && m_engine->itemAvailable());
    m_runBtn->setEnabled(on);
    updateCursor();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Cursor navigation
// ─────────────────────────────────────────────────────────────────────────────

void LevelBattleWidget::moveCursor(int delta)
{
    QPushButton* btns[4] = {m_fightBtn, m_specialBtn, m_itemBtn, m_runBtn};
    int next = m_cursorIndex;
    do {
        next = (next + delta + 4) % 4;
    } while (!btns[next]->isEnabled() && next != m_cursorIndex);
    m_cursorIndex = next;
    updateCursor();
}

void LevelBattleWidget::updateCursor()
{
    const QStringList base = {"FIGHT", "SPECIAL", "ITEM", "RUN"};
    QPushButton* btns[4] = {m_fightBtn, m_specialBtn, m_itemBtn, m_runBtn};
    for (int i = 0; i < 4; ++i)
        btns[i]->setText((i == m_cursorIndex ? "► " : "  ") + base[i]);
}

void LevelBattleWidget::confirmSelection()
{
    switch (m_cursorIndex) {
    case 0: m_engine->onPlayerChoseFight();   break;
    case 1: m_engine->onPlayerChoseSpecial(); break;
    case 2: m_itemBtn->click();               break;
    case 3: m_engine->onPlayerChoseRun();     break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Paint — cinematic theme background
// ─────────────────────────────────────────────────────────────────────────────

void LevelBattleWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    const int w = width(), h = height();

    // Base dark background
    // Draw level background image
    QString bgPath;
    switch (m_theme) {
    case LevelTheme::Cave:    bgPath = ":/backgrounds/cave_level.jpg";    break;
    case LevelTheme::Forest:  bgPath = ":/backgrounds/forest_level.jpg";  break;
    case LevelTheme::Peak:    bgPath = ":/backgrounds/mountain_level.jpg"; break;
    case LevelTheme::Volcano: bgPath = ":/backgrounds/volcano_level.jpg"; break;
    default:                  bgPath = ":/backgrounds/battle_bg.png";      break;
    }
    QPixmap bg(bgPath);
    if (!bg.isNull())
        p.drawPixmap(rect(), bg);
    else
        p.fillRect(rect(), themeBgColor());
    // Theme overlay gradient — darker at edges, lighter center
    p.fillRect(rect(), themeOverlay());

    // Decorative vertical separator between enemy and player halves
    QColor sep = themeColor();
    sep.setAlpha(60);
    p.fillRect(w/2 - 1, 80, 2, h - 160, sep);

    // Bottom button bar background
    p.fillRect(0, h - 90, w, 90, QColor(0, 0, 0, 180));
    p.setPen(QPen(themeColor(), 1));
    p.drawLine(0, h - 90, w, h - 90);

    // Top header bar
    p.fillRect(0, 0, w, 70, QColor(0, 0, 0, 160));
    p.drawLine(0, 70, w, 70);

    // Corner accents
    const int cs = 20;
    p.setPen(QPen(themeColor(), 2));
    p.drawLine(0, 0, cs, 0);   p.drawLine(0, 0, 0, cs);
    p.drawLine(w-cs, 0, w, 0); p.drawLine(w, 0, w, cs);
    p.drawLine(0, h-cs, 0, h); p.drawLine(0, h, cs, h);
    p.drawLine(w, h-cs, w, h); p.drawLine(w-cs, h, w, h);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Layout
// ─────────────────────────────────────────────────────────────────────────────

void LevelBattleWidget::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    layoutChildren();
    if (m_pauseOverlay) m_pauseOverlay->resize(size());
    if (m_itemMenu) {
        int mx = (width()  - m_itemMenu->width())  / 2;
        int my = height()  - 90 - m_itemMenu->height() - 8;
        m_itemMenu->move(mx, my);
    }
}

void LevelBattleWidget::layoutChildren()
{
    const int w = width(), h = height();
    const int midY = 80;   // below header bar

    // ── Header ────────────────────────────────────────────────────────────────
    m_headerLabel->setGeometry(0, 8, w, 28);
    m_subheaderLabel->setGeometry(0, 38, w, 20);
    m_scoreLabel->setGeometry(0, 58, w, 16);

    // ── Battle area height ────────────────────────────────────────────────────
    const int battleH = h - midY - 90;   // between header and button bar

    // ── Enemy (left half) ─────────────────────────────────────────────────────
    const int halfW = w / 2;
    m_enemyNameLabel->setGeometry(0, midY + 8, halfW, 24);
    m_enemyHP->move(halfW/2 - 110, midY + 36);
    m_enemySP->move(halfW/2 - 110, midY + 56);
    m_enemySprite->move(halfW/2 - 100, midY + 70);

    // ── Player (right half) ───────────────────────────────────────────────────
    m_playerNameLabel->setGeometry(halfW, midY + 8, halfW, 24);
    m_playerHP->move(halfW + halfW/2 - 110, midY + 36);
    m_playerSP->move(halfW + halfW/2 - 110, midY + 56);
    m_playerSprite->move(halfW + halfW/2 - 100, midY + 70);

    // ── Message ───────────────────────────────────────────────────────────────
    m_messageLabel->setGeometry(40, h - 96, w - 80, 20);

    // ── Buttons ───────────────────────────────────────────────────────────────
    const int btnY    = h - 82;
    const int btnW    = 160;
    const int spacing = (w - btnW * 4) / 5;

    m_fightBtn->setGeometry  (spacing,             btnY, btnW, 36);
    m_specialBtn->setGeometry(spacing*2 + btnW,    btnY, btnW, 36);
    m_itemBtn->setGeometry   (spacing*3 + btnW*2,  btnY, btnW, 36);
    m_runBtn->setGeometry    (spacing*4 + btnW*3,  btnY, btnW, 36);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Input
// ─────────────────────────────────────────────────────────────────────────────

void LevelBattleWidget::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) { m_engine->onPauseToggle(); return; }
    if (!m_fightBtn->isEnabled())   { QWidget::keyPressEvent(e); return; }

    switch (e->key()) {
    case Qt::Key_Left:               moveCursor(-1);       break;
    case Qt::Key_Right:              moveCursor(+1);       break;
    case Qt::Key_Return:
    case Qt::Key_Space:              confirmSelection();    break;
    default: QWidget::keyPressEvent(e);
    }
}

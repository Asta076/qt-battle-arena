#include "onlinepvpwidget.h"
#include "warrior.h"
#include "mage.h"
#include "archer.h"
#include "spritecache.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QtMath>

// ── Key bitmask encoding ──────────────────────────────────────────────────────
// bit 0 = W, 1 = S, 2 = A, 3 = D, 4 = Up, 5 = Down, 6 = Left, 7 = Right
// bit 8 = J (attack), 9 = K (special)
static constexpr uint32_t KEY_W      = 1 << 0;
static constexpr uint32_t KEY_S      = 1 << 1;
static constexpr uint32_t KEY_A      = 1 << 2;
static constexpr uint32_t KEY_D      = 1 << 3;
static constexpr uint32_t KEY_UP     = 1 << 4;
static constexpr uint32_t KEY_DOWN   = 1 << 5;
static constexpr uint32_t KEY_LEFT   = 1 << 6;
static constexpr uint32_t KEY_RIGHT  = 1 << 7;
static constexpr uint32_t KEY_ATTACK  = 1 << 8;
static constexpr uint32_t KEY_SPECIAL = 1 << 9;

// ─────────────────────────────────────────────────────────────────────────────

static Character* makeCharacter(CharacterType type, const QString& name)
{
    switch (type) {
    case CharacterType::Mage:   return new Mage  (name);
    case CharacterType::Archer: return new Archer (name);
    default:                    return new Warrior(name);
    }
}

static void loadSheet(DungeonSpriteSheet& sheet,
                      DungeonAttackSheet& atkSheet,
                      CharacterType type)
{
    QString mov, atk;
    switch (type) {
    case CharacterType::Mage:
        mov = ":/sprites/mage_movement_8dir_6frames.png";
        atk = ":/sprites/mage_attack_8dir_7frames.png";
        break;
    case CharacterType::Archer:
        mov = ":/sprites/archer_movement_8dir_6frames.png";
        atk = ":/sprites/archer_attack_8dir_7frames.png";
        break;
    default:
        mov = ":/sprites/warrior_movement_8dir_6frames.png";
        atk = ":/sprites/warrior_attack_8dir_7frames.png";
        break;
    }
    sheet.pixmap    = SpriteCache::instance().get(mov);
    atkSheet.pixmap = SpriteCache::instance().get(atk);
}

static QSet<int> keysFromMask(uint32_t mask)
{
    QSet<int> keys;
    if (mask & KEY_W)    keys.insert(Qt::Key_W);
    if (mask & KEY_S)    keys.insert(Qt::Key_S);
    if (mask & KEY_A)    keys.insert(Qt::Key_A);
    if (mask & KEY_D)    keys.insert(Qt::Key_D);
    if (mask & KEY_UP)   keys.insert(Qt::Key_Up);
    if (mask & KEY_DOWN) keys.insert(Qt::Key_Down);
    if (mask & KEY_LEFT) keys.insert(Qt::Key_Left);
    if (mask & KEY_RIGHT)keys.insert(Qt::Key_Right);
    return keys;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────

OnlinePvpWidget::OnlinePvpWidget(AudioManager* audio, QWidget* parent)
    : QWidget(parent), m_audio(audio)
{
    // Keep the view in a layout — this is the safe approach
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_scene = new QGraphicsScene(0, 0, WORLD_W, WORLD_H, this);

    m_view = new QGraphicsView(m_scene, this);
    m_view->setFrameShape(QFrame::NoFrame);
    m_view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(m_view);

    m_ticker.setInterval(16);
    connect(&m_ticker, &QTimer::timeout, this, &OnlinePvpWidget::onTick);

    m_voteTimer.setInterval(1000);
    connect(&m_voteTimer, &QTimer::timeout, this, &OnlinePvpWidget::onVoteTimerTick);

    setFocusPolicy(Qt::StrongFocus);
    buildHud();   // HUD floats on top as an overlay child
}

OnlinePvpWidget::~OnlinePvpWidget()
{
    delete m_localChar;
    delete m_remoteChar;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Activate
// ─────────────────────────────────────────────────────────────────────────────

void OnlinePvpWidget::activate(CharacterType localChar, CharacterType remoteChar,
                               bool isHost, NetworkManager* network)
{
    m_isHost  = isHost;
    m_network = network;
    m_seq     = 0;
    m_running = true;

    m_localScore  = 0;
    m_remoteScore = 0;
    m_remoteKeyMask = 0;

    m_localAttackPending  = false;
    m_localSpecialPending = false;
    m_remoteAttackPending  = false;
    m_remoteSpecialPending = false;

    m_heldKeys.clear();

    // Create character logic
    delete m_localChar;
    delete m_remoteChar;
    m_localChar  = makeCharacter(localChar,  "You");
    m_remoteChar = makeCharacter(remoteChar, "Opponent");

    // Load sprites
    loadSheet(m_localSheet,  m_localAtkSheet,  localChar);
    loadSheet(m_remoteSheet, m_remoteAtkSheet, remoteChar);

    // Build scene
    m_scene->clear();
    m_localSprite  = nullptr;
    m_remoteSprite = nullptr;
    buildScene(localChar, remoteChar);
    placeSprites();
    updateHud();

    // Connect network signals — disconnect first to avoid double-connecting
    disconnect(m_network, nullptr, this, nullptr);
    connect(m_network, &NetworkManager::messageReceived,
            this, &OnlinePvpWidget::onMessageReceived);
    connect(m_network, &NetworkManager::disconnected,
            this, &OnlinePvpWidget::onDisconnected);

    m_roundOverlay->hide();
    m_voteLabel->hide();
    m_nextRoundBtn->hide();
    m_leaveBtn->hide();

    m_ticker.start();
    setFocus();
    QTimer::singleShot(0, this, [this]{ fitView(); });


    QTimer::singleShot(100, this, [this]{
        if (m_audio) m_audio->playMusic("/music/battle.ogg");
    });}

void OnlinePvpWidget::deactivate()
{
    m_ticker.stop();
    m_voteTimer.stop();
    m_running = false;
    m_heldKeys.clear();
    if (m_network) disconnect(m_network, nullptr, this, nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Scene
// ─────────────────────────────────────────────────────────────────────────────

void OnlinePvpWidget::buildScene(CharacterType localChar, CharacterType remoteChar)
{
    // Background
    QPixmap bg = SpriteCache::instance().get(":/backgrounds/pvp_arena.png");
    if (!bg.isNull()) {
        auto* bgItem = m_scene->addPixmap(
            bg.scaled(WORLD_W, WORLD_H, Qt::IgnoreAspectRatio,
                      Qt::SmoothTransformation));
        bgItem->setZValue(0);
    } else {
        m_scene->addRect(0, 0, WORLD_W, WORLD_H,
                         Qt::NoPen, QBrush(QColor("#1a1a2e")))->setZValue(0);
    }

    // Arena floor line
    m_scene->addLine(0, WORLD_H - 60, WORLD_W, WORLD_H - 60,
                     QPen(QColor("#4a4a6a"), 3))->setZValue(1);

    // Player sprites
    m_localSprite  = new DungeonPlayerSprite(m_localSheet,  m_localAtkSheet);
    m_remoteSprite = new DungeonPlayerSprite(m_remoteSheet, m_remoteAtkSheet);

    m_localSprite ->setZValue(9);
    m_remoteSprite->setZValue(9);

    // Mirror remote sprite so they face each other
    //m_remoteSprite->setTransform(QTransform().scale(-1, 1));

    m_scene->addItem(m_localSprite);
    m_scene->addItem(m_remoteSprite);

    // Name labels
    auto* localLbl = m_scene->addText(
        m_isHost ? "YOU (P1)" : "YOU (P2)",
        QFont("Press Start 2P", 7));
    localLbl->setDefaultTextColor(QColor("#44AAFF"));
    localLbl->setZValue(10);

    auto* remoteLbl = m_scene->addText(
        m_isHost ? "OPPONENT (P2)" : "OPPONENT (P1)",
        QFont("Press Start 2P", 7));
    remoteLbl->setDefaultTextColor(QColor("#FF4444"));
    remoteLbl->setZValue(10);

    // Controls hint
    auto* hint = m_scene->addText(
        "WASD/Arrows=move   J=attack   K=special",
        QFont("Arial", 6));
    hint->setDefaultTextColor(QColor("#ffffff66"));
    hint->setPos(8, WORLD_H - 18);
    hint->setZValue(10);
}

void OnlinePvpWidget::placeSprites()
{
    if (m_localSprite)
        m_localSprite->setPos(150, WORLD_H - 120);
    if (m_remoteSprite) {
        // Mirror is applied via transform, so X position is from right edge
        m_remoteSprite->setPos(WORLD_W - 150 - DungeonPlayerSprite::W, WORLD_H - 120);
    }
    m_localFacing  = Direction::Right;
    m_remoteFacing = Direction::Left;
}

void OnlinePvpWidget::buildHud()
{
    m_hudWidget = new QWidget(this);
    m_hudWidget->setFixedHeight(60);
    m_hudWidget->setAttribute(Qt::WA_StyledBackground, true);
    m_hudWidget->setStyleSheet(
        "QWidget { background-color: rgba(13,13,26,200); border-bottom: 2px solid #F0E8D0; }"
        "QLabel  { background: transparent; border: none; color: #F0E8D0; font-size: 8px; }");

    auto* row = new QHBoxLayout(m_hudWidget);
    row->setContentsMargins(12, 4, 12, 4);
    row->setSpacing(8);

    // Local HP
    m_localHpLabel = new QLabel("YOU  100/100", m_hudWidget);
    m_localHpLabel->setFixedWidth(120);
    m_localHpBar = new HealthBarWidget(m_hudWidget);
    m_localHpBar->setFixedSize(200, 20);
    m_localHpBar->setFixedBarColor(QColor("#44AAFF"));

    // Score
    m_scoreLabel = new QLabel("0 — 0", m_hudWidget);
    m_scoreLabel->setAlignment(Qt::AlignCenter);
    m_scoreLabel->setStyleSheet("color: #FFD700; font-size: 10px; background: transparent; border: none;");

    // Remote HP
    m_remoteHpBar = new HealthBarWidget(m_hudWidget);
    m_remoteHpBar->setFixedSize(200, 20);
    m_remoteHpBar->setFixedBarColor(QColor("#FF4444"));
    m_remoteHpLabel = new QLabel("OPP  100/100", m_hudWidget);
    m_remoteHpLabel->setFixedWidth(120);
    m_remoteHpLabel->setAlignment(Qt::AlignRight);

    row->addWidget(m_localHpLabel);
    row->addWidget(m_localHpBar);
    row->addStretch();
    row->addWidget(m_scoreLabel);
    row->addStretch();
    row->addWidget(m_remoteHpBar);
    row->addWidget(m_remoteHpLabel);
    m_hudWidget->raise();

    // Round over overlay label
    m_roundOverlay = new QLabel("", this);
    m_roundOverlay->setAlignment(Qt::AlignCenter);
    m_roundOverlay->setStyleSheet(
        "QLabel { color: #FFD700; font-size: 14px; background: rgba(0,0,0,160);"
        "         border: 2px solid #FFD700; padding: 12px; }");
    m_roundOverlay->setFont(QFont("Press Start 2P", 14));
    m_roundOverlay->hide();

    // Vote label (countdown)
    m_voteLabel = new QLabel("", this);
    m_voteLabel->setAlignment(Qt::AlignCenter);
    m_voteLabel->setStyleSheet(
        "QLabel { color: #F0E8D0; font-size: 8px; background: transparent; border: none; }");
    m_voteLabel->setFont(QFont("Press Start 2P", 8));
    m_voteLabel->hide();

    // Next round button
    m_nextRoundBtn = new QPushButton("► NEXT ROUND", this);
    m_nextRoundBtn->setFixedWidth(200);
    m_nextRoundBtn->hide();
    connect(m_nextRoundBtn, &QPushButton::clicked, this, [this]{
        m_localVoted = true;
        m_nextRoundBtn->setEnabled(false);
        m_nextRoundBtn->setText("Waiting...");
        m_network->send(NM::build_next_vote(true));
    });

    // Leave button
    m_leaveBtn = new QPushButton("← LEAVE", this);
    m_leaveBtn->setFixedWidth(160);
    m_leaveBtn->hide();
    connect(m_leaveBtn, &QPushButton::clicked, this, [this]{
        m_network->send(NM::build_next_vote(false));
        deactivate();
        emit returnToRoom();
    });
}

void OnlinePvpWidget::updateHud()
{
    if (!m_localChar || !m_remoteChar) return;

    m_localHpLabel->setText(
        QString("YOU  %1/%2")
            .arg(static_cast<int>(m_localChar->getHealthPercent() * m_localChar->getMaxHealth()))
            .arg(m_localChar->getMaxHealth()));

    m_remoteHpLabel->setText(
        QString("OPP  %1/%2")
            .arg(static_cast<int>(m_remoteChar->getHealthPercent() * m_remoteChar->getMaxHealth()))
            .arg(m_remoteChar->getMaxHealth()));
    m_remoteHpBar->animateTo(m_remoteChar->getHealthPercent());
    m_localHpBar->animateTo(m_localChar->getHealthPercent());
    m_scoreLabel->setText(
        QString("%1 — %2").arg(m_localScore).arg(m_remoteScore));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Game loop
// ─────────────────────────────────────────────────────────────────────────────

void OnlinePvpWidget::onTick()
{
    if (!m_running) return;
    ++m_seq;

    if (m_isHost) {
        // ── HOST: simulate everything, send state ─────────────────────────────

        // Move local sprite
        QPointF vel = m_controller.computeVelocity(m_heldKeys);
        QPointF lPos = m_localSprite->pos() + vel;
        lPos.setX(qBound(0.0, lPos.x(), WORLD_W - DungeonPlayerSprite::W));
        lPos.setY(qBound(60.0, lPos.y(), WORLD_H - DungeonPlayerSprite::H - 10.0));
        m_localSprite->setPos(lPos);

        bool lMoving = m_controller.isMoving(m_heldKeys);
        Direction lDir = m_controller.computeDirection(m_heldKeys);
        m_localSprite->updateAnimation(lMoving, lDir);
        if (lMoving) m_localFacing = lDir;

        // Move remote sprite based on their last known keys
        QSet<int> remoteKeys = keysFromMask(m_remoteKeyMask);
        QPointF rVel = m_controller.computeVelocity(remoteKeys);
        QPointF rPos = m_remoteSprite->pos() + rVel;
        rPos.setX(qBound(0.0, rPos.x(), WORLD_W - DungeonPlayerSprite::W));
        rPos.setY(qBound(60.0, rPos.y(), WORLD_H - DungeonPlayerSprite::H - 10.0));
        m_remoteSprite->setPos(rPos);

        bool rMoving = !remoteKeys.isEmpty();
        Direction rDir = m_controller.computeDirection(remoteKeys);
        m_remoteSprite->updateAnimation(rMoving, rDir);
        if (rMoving) m_remoteFacing = rDir;

        // Process pending attacks
        if (m_localAttackPending)  { handleAttack(true);   m_localAttackPending  = false; }
        if (m_localSpecialPending) { handleSpecial(true);  m_localSpecialPending = false; }


        // Remote attack flags from input message
        if (m_remoteKeyMask & KEY_ATTACK)  { handleAttack(false);  }
        if (m_remoteKeyMask & KEY_SPECIAL) { handleSpecial(false); }

        // Clear attack bits from mask after processing — movement bits stay
        m_remoteKeyMask &= ~KEY_ATTACK;
        m_remoteKeyMask &= ~KEY_SPECIAL;

        checkCombat();
        updateHud();

        // Send authoritative state every tick (~60/s)
        m_network->send(NM::build_state(
            lPos.x(), lPos.y(),
            static_cast<int>(m_localChar->getHealthPercent() * m_localChar->getMaxHealth()),
            static_cast<int>(m_localChar->getSpPercent() * 100),
            rPos.x(), rPos.y(),
            static_cast<int>(m_remoteChar->getHealthPercent() * m_remoteChar->getMaxHealth()),
            static_cast<int>(m_remoteChar->getSpPercent() * 100),
            m_seq));

    } else {
        // ── GUEST: send input, apply received state from host ─────────────────
        sendInputToHost();
        // State is applied in onMessageReceived when STATE messages arrive
    }
}

uint32_t OnlinePvpWidget::buildKeyMask() const
{
    uint32_t mask = 0;
    if (m_heldKeys.contains(Qt::Key_W)     || m_heldKeys.contains(Qt::Key_Up))    mask |= KEY_W;
    if (m_heldKeys.contains(Qt::Key_S)     || m_heldKeys.contains(Qt::Key_Down))  mask |= KEY_S;
    if (m_heldKeys.contains(Qt::Key_A)     || m_heldKeys.contains(Qt::Key_Left))  mask |= KEY_A;
    if (m_heldKeys.contains(Qt::Key_D)     || m_heldKeys.contains(Qt::Key_Right)) mask |= KEY_D;
    if (m_localAttackPending)  mask |= KEY_ATTACK;
    if (m_localSpecialPending) mask |= KEY_SPECIAL;
    return mask;
}

void OnlinePvpWidget::sendInputToHost()
{
    uint32_t mask = buildKeyMask();
    m_network->send(NM::build_input(mask, m_seq));
    // Clear one-shot attack flags after sending
    m_localAttackPending  = false;
    m_localSpecialPending = false;
}

void OnlinePvpWidget::applyReceivedState(const std::string& msg)
{
    // Guest applies host's authoritative state
    float p1x  = NM::extract_float(msg, "p1x");
    float p1y  = NM::extract_float(msg, "p1y");
    int   p1hp = NM::extract_int  (msg, "p1hp");
    float p2x  = NM::extract_float(msg, "p2x");
    float p2y  = NM::extract_float(msg, "p2y");
    int   p2hp = NM::extract_int  (msg, "p2hp");

    // Guest is P2, host is P1
    // Local = P2 (guest), remote = P1 (host)
    if (m_localSprite)  m_localSprite ->setPos(p2x, p2y);
    if (m_remoteSprite) m_remoteSprite->setPos(p1x, p1y);

    if (m_localChar)  m_localChar ->setHealthDirect(p2hp);
    if (m_remoteChar) m_remoteChar->setHealthDirect(p1hp);

    updateHud();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Combat (host only)
// ─────────────────────────────────────────────────────────────────────────────

void OnlinePvpWidget::checkCombat()
{
    if (!m_localChar || !m_remoteChar) return;
    if (!m_localChar->isAlive()) { endRound(2); return; }
    if (!m_remoteChar->isAlive()) { endRound(1); return; }
}

void OnlinePvpWidget::handleAttack(bool isLocal)
{
    // Simple range check — if sprites overlap enough, deal damage
    Character* attacker = isLocal ? m_localChar  : m_remoteChar;
    Character* defender = isLocal ? m_remoteChar : m_localChar;
    DungeonPlayerSprite* atkSprite = isLocal ? m_localSprite  : m_remoteSprite;
    DungeonPlayerSprite* defSprite = isLocal ? m_remoteSprite : m_localSprite;

    if (!attacker || !defender || !atkSprite || !defSprite) return;

    // Attack animation
    atkSprite->startAttackAnimation(isLocal ? m_localFacing : m_remoteFacing);

    // Range check — 150px
    QPointF delta = atkSprite->pos() - defSprite->pos();
    qreal dist = qSqrt(delta.x() * delta.x() + delta.y() * delta.y());
    if (dist > 150.0) return;

    int dmg = attacker->attack();
    defender->takeDamage(dmg);
    attacker->addSpFromAttack();

    if (m_audio) m_audio->playSfx("/sfx/hit.wav");
}

void OnlinePvpWidget::handleSpecial(bool isLocal)
{
    Character* attacker = isLocal ? m_localChar  : m_remoteChar;
    Character* defender = isLocal ? m_remoteChar : m_localChar;
    DungeonPlayerSprite* atkSprite = isLocal ? m_localSprite  : m_remoteSprite;
    DungeonPlayerSprite* defSprite = isLocal ? m_remoteSprite : m_localSprite;

    if (!attacker || !defender || !atkSprite || !defSprite) return;
    if (!attacker->canUseSpecial()) return;

    atkSprite->startAttackAnimation(isLocal ? m_localFacing : m_remoteFacing);

    QPointF delta = atkSprite->pos() - defSprite->pos();
    qreal dist = qSqrt(delta.x() * delta.x() + delta.y() * delta.y());
    if (dist > 250.0) return;   // specials have longer range

    int dmg = attacker->specialAbility();
    defender->takeDamage(dmg);

    if (m_audio) m_audio->playSfx("/sfx/special.wav");
}

void OnlinePvpWidget::endRound(int winner)
{
    if (!m_running) return;
    m_ticker.stop();

    if (winner == 1) {
        m_localScore++;
        showRoundOverlay("YOU WIN!");
    } else {
        m_remoteScore++;
        showRoundOverlay("YOU LOSE!");
    }

    // Host sends round_end to both (server relays it back too)
    if (m_isHost)
        m_network->send(NM::build_round_end(winner));

    if (m_audio) m_audio->playSfx(winner == 1 ? "/sfx/faint.wav" : "/sfx/faint.wav");

    startVotePhase();
}

void OnlinePvpWidget::showRoundOverlay(const QString& text)
{
    m_roundOverlay->setText(text);
    m_roundOverlay->adjustSize();
    m_roundOverlay->move((width()  - m_roundOverlay->width())  / 2,
                         (height() - m_roundOverlay->height()) / 2 - 40);
    m_roundOverlay->show();
    m_roundOverlay->raise();
}

void OnlinePvpWidget::startVotePhase()
{
    m_localVoted  = false;
    m_remoteVoted = false;
    m_voteCountdown = 20;

    m_nextRoundBtn->setText("► NEXT ROUND");
    m_nextRoundBtn->setEnabled(true);
    m_nextRoundBtn->show();
    m_leaveBtn->show();

    m_voteLabel->setText("Next round? 20s remaining");
    m_voteLabel->show();

    // Position buttons below the overlay
    m_nextRoundBtn->move((width() - m_nextRoundBtn->width()) / 2,
                          height() / 2 + 20);
    m_leaveBtn->move((width() - m_leaveBtn->width()) / 2,
                      height() / 2 + 64);
    m_voteLabel->move(0, height() / 2 - 10);
    m_voteLabel->setFixedWidth(width());

    m_nextRoundBtn->raise();
    m_leaveBtn->raise();
    m_voteLabel->raise();

    m_voteTimer.start();
}

void OnlinePvpWidget::onVoteTimerTick()
{
    --m_voteCountdown;

    m_voteLabel->setText(
        QString("Next round? %1s remaining").arg(m_voteCountdown));

    if (m_voteCountdown <= 0) {
        m_voteTimer.stop();
        // Timer expired — go back to room regardless of votes
        m_network->send(NM::build_next_vote(false));
        deactivate();
        emit returnToRoom();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Network messages
// ─────────────────────────────────────────────────────────────────────────────

void OnlinePvpWidget::onMessageReceived(const QString& qmsg)
{
    std::string msg  = qmsg.toStdString();
    std::string type = NM::extract_type(msg);

    if (type == NM::INPUT && m_isHost) {
        // Guest sent their keys — decode and store for next tick
        m_remoteKeyMask = static_cast<uint32_t>(NM::extract_int(msg, "keys"));

    } else if (type == NM::STATE && !m_isHost) {
        // Host sent authoritative state — apply to scene
        applyReceivedState(msg);

    } else if (type == NM::ROUND_END && !m_isHost) {
        // Host told us who won
        int winner = NM::extract_int(msg, "winner");
        // winner=1 means host(P1) won → guest(local) lost
        endRound(winner == 1 ? 2 : 1);

    } else if (type == NM::NEXT_RESULT) {
        bool cont = NM::extract_bool(msg, "continue");
        m_voteTimer.stop();
        m_nextRoundBtn->hide();
        m_leaveBtn->hide();
        m_voteLabel->hide();
        m_roundOverlay->hide();

        if (cont) {
            // Start next round — reset characters and respawn
            m_localChar->resetHealth();
            m_remoteChar->resetHealth();
            placeSprites();
            updateHud();
            m_seq = 0;
            m_remoteKeyMask = 0;
            m_running = true;
            m_ticker.start();
        } else {
            deactivate();
            emit returnToRoom();
        }

    } else if (type == NM::NEXT_VOTE && m_isHost) {
        // Host receives guest's vote
        bool vote = NM::extract_bool(msg, "vote");
        m_remoteVoted = vote;

        if (!vote) {
            // Guest voted no — send result immediately
            m_voteTimer.stop();
            m_network->send(NM::build_next_result(false));
            deactivate();
            emit returnToRoom();
        } else if (m_localVoted) {
            // Both voted yes
            m_voteTimer.stop();
            m_network->send(NM::build_next_result(true));
        }
        // If host hasn't voted yet, wait

    } else if (type == NM::ROOM_CLOSED) {
        m_voteTimer.stop();
        m_ticker.stop();
        showRoundOverlay("Opponent disconnected.");
        QTimer::singleShot(2000, this, [this]{
            deactivate();
            emit returnToRoom();
        });
    }
}

void OnlinePvpWidget::onDisconnected(const QString&)
{
    m_voteTimer.stop();
    m_ticker.stop();
    showRoundOverlay("Connection lost.");
    QTimer::singleShot(2000, this, [this]{
        deactivate();
        emit returnToRoom();
    });
}

// ─────────────────────────────────────────────────────────────────────────────
//  Input
// ─────────────────────────────────────────────────────────────────────────────

void OnlinePvpWidget::keyPressEvent(QKeyEvent* e)
{
    if (e->isAutoRepeat()) return;

    if (e->key() == Qt::Key_J && !e->isAutoRepeat()) {
        m_localAttackPending = true;
        if (m_isHost) { handleAttack(true); m_localAttackPending = false; }
        return;
    }
    if (e->key() == Qt::Key_K && !e->isAutoRepeat()) {
        m_localSpecialPending = true;
        if (m_isHost) { handleSpecial(true); m_localSpecialPending = false; }
        return;
    }

    m_heldKeys.insert(e->key());
}

void OnlinePvpWidget::keyReleaseEvent(QKeyEvent* e)
{
    if (!e->isAutoRepeat())
        m_heldKeys.remove(e->key());
}

void OnlinePvpWidget::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);

    // HUD floats at top as overlay — same pattern as GoldHudWidget
    if (m_hudWidget)
        m_hudWidget->setGeometry(0, 0, width(), 60);

    if (m_hudWidget) m_hudWidget->raise();

    QTimer::singleShot(0, this, [this]{ fitView(); });
}

void OnlinePvpWidget::fitView()
{
    if (m_view)
        m_view->fitInView(0, 0, WORLD_W, WORLD_H, Qt::IgnoreAspectRatio);
}

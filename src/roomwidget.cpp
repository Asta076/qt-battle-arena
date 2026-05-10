#include "roomwidget.h"
#include "networkmessage.h"
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QMessageBox>

RoomWidget::RoomWidget(QWidget* parent)
    : QWidget(parent)
{
    m_network = new NetworkManager(this);

    connect(m_network, &NetworkManager::connected,
            this, &RoomWidget::onConnected);
    connect(m_network, &NetworkManager::connectionFailed,
            this, &RoomWidget::onConnectionFailed);
    connect(m_network, &NetworkManager::disconnected,
            this, &RoomWidget::onDisconnected);
    connect(m_network, &NetworkManager::messageReceived,
            this, &RoomWidget::onMessageReceived);

    buildUI();
}

RoomWidget::~RoomWidget()
{
    m_network->shutdown();
}

void RoomWidget::buildUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(60, 40, 60, 40);
    root->setSpacing(16);
    root->setAlignment(Qt::AlignCenter);

    // ── Title ─────────────────────────────────────────────────────────────────
    auto* title = new QLabel("ONLINE PVP", this);
    title->setObjectName("titleLabel");
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);
    root->addSpacing(12);

    // ── Connection fields ─────────────────────────────────────────────────────
    auto* grid = new QGridLayout;
    grid->setSpacing(8);

    auto addRow = [&](int row, const QString& label, QLineEdit*& field,
                      const QString& placeholder) {
        auto* lbl = new QLabel(label, this);
        lbl->setObjectName("subtitleLabel");
        field = new QLineEdit(this);
        field->setPlaceholderText(placeholder);
        field->setFixedWidth(220);
        grid->addWidget(lbl,   row, 0, Qt::AlignRight);
        grid->addWidget(field, row, 1, Qt::AlignLeft);
    };

    addRow(0, "SERVER:",    m_hostEdit, "localhost");
    addRow(1, "PORT:",      m_portEdit, "55000");
    addRow(2, "ROOM CODE:", m_roomEdit, "e.g. ABC123");
    addRow(3, "YOUR NAME:", m_nameEdit, "Player");

    m_hostEdit->setText("localhost");
    m_portEdit->setText("55000");

    root->addLayout(grid);

    // ── Character selection ───────────────────────────────────────────────────
    auto* charRow = new QHBoxLayout;
    charRow->setAlignment(Qt::AlignCenter);
    charRow->setSpacing(12);

    auto* charLbl = new QLabel("CHARACTER:", this);
    charLbl->setObjectName("subtitleLabel");

    m_charCombo = new QComboBox(this);
    m_charCombo->addItem("Warrior");
    m_charCombo->addItem("Mage");
    m_charCombo->addItem("Archer");
    m_charCombo->setFixedWidth(140);

    charRow->addWidget(charLbl);
    charRow->addWidget(m_charCombo);
    root->addLayout(charRow);
    root->addSpacing(8);

    // ── Room status ───────────────────────────────────────────────────────────
    m_roomLabel = new QLabel("", this);
    m_roomLabel->setObjectName("subtitleLabel");
    m_roomLabel->setAlignment(Qt::AlignCenter);
    m_roomLabel->setStyleSheet("color: #FFD700;");
    root->addWidget(m_roomLabel);

    // ── Player slots ──────────────────────────────────────────────────────────
    auto* slotS = new QHBoxLayout;
    slotS->setAlignment(Qt::AlignCenter);
    slotS->setSpacing(40);

    m_p1Label = new QLabel("P1: —", this);
    m_p2Label = new QLabel("P2: —", this);
    m_p1Label->setObjectName("subtitleLabel");
    m_p2Label->setObjectName("subtitleLabel");
    m_p1Label->setAlignment(Qt::AlignCenter);
    m_p2Label->setAlignment(Qt::AlignCenter);

    slotS->addWidget(m_p1Label);
    slotS->addWidget(m_p2Label);
    root->addLayout(slotS);

    // ── Status label ──────────────────────────────────────────────────────────
    m_statusLabel = new QLabel("Enter server details and a room code.", this);
    m_statusLabel->setObjectName("subtitleLabel");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);
    root->addWidget(m_statusLabel);
    root->addSpacing(8);

    // ── Buttons ───────────────────────────────────────────────────────────────
    auto* btnRow = new QHBoxLayout;
    btnRow->setAlignment(Qt::AlignCenter);
    btnRow->setSpacing(16);

    m_connectBtn = new QPushButton("CONNECT", this);
    m_readyBtn   = new QPushButton("READY",   this);
    m_backBtn    = new QPushButton("← BACK",  this);

    m_connectBtn->setFixedWidth(160);
    m_readyBtn  ->setFixedWidth(160);
    m_backBtn   ->setFixedWidth(160);

    m_readyBtn->setEnabled(false);

    connect(m_connectBtn, &QPushButton::clicked, this, &RoomWidget::onConnectClicked);
    connect(m_readyBtn,   &QPushButton::clicked, this, &RoomWidget::onReadyClicked);
    connect(m_backBtn,    &QPushButton::clicked, this, &RoomWidget::onBackClicked);

    btnRow->addWidget(m_connectBtn);
    btnRow->addWidget(m_readyBtn);
    btnRow->addWidget(m_backBtn);
    root->addLayout(btnRow);
}

void RoomWidget::activate()
{
    setPhase(Phase::Disconnected);
    m_p1Label->setText("P1: —");
    m_p2Label->setText("P2: —");
    m_roomLabel->setText("");
    setStatus("Enter server details and a room code.");
}

void RoomWidget::deactivate()
{
    // Don't shut down the network here — the game widget still needs it
}

void RoomWidget::setStatus(const QString& text, const QString& color)
{
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(
        QString("color: %1; background: transparent; border: none;").arg(color));
}

void RoomWidget::setPhase(Phase p)
{
    m_phase = p;

    m_connectBtn->setEnabled(p == Phase::Disconnected);
    m_readyBtn  ->setEnabled(p == Phase::Selecting);
    m_hostEdit  ->setEnabled(p == Phase::Disconnected);
    m_portEdit  ->setEnabled(p == Phase::Disconnected);
    m_roomEdit  ->setEnabled(p == Phase::Disconnected);
    m_nameEdit  ->setEnabled(p == Phase::Disconnected);
    m_charCombo ->setEnabled(p == Phase::Selecting);
}

// ─────────────────────────────────────────────────────────────────────────────
//  UI slots
// ─────────────────────────────────────────────────────────────────────────────

void RoomWidget::onConnectClicked()
{
    QString host = m_hostEdit->text().trimmed();
    QString port = m_portEdit->text().trimmed();
    QString room = m_roomEdit->text().trimmed().toUpper();
    QString name = m_nameEdit->text().trimmed();

    if (host.isEmpty()) host = "localhost";
    if (port.isEmpty()) port = "55000";
    if (room.isEmpty()) { setStatus("Please enter a room code.", "#EE3333"); return; }
    if (name.isEmpty()) name = "Player";

    bool ok;
    quint16 portNum = port.toUShort(&ok);
    if (!ok) { setStatus("Invalid port number.", "#EE3333"); return; }

    setPhase(Phase::Connecting);
    setStatus("Connecting to " + host + ":" + port + "...", "#FFE066");

    m_network->connectToServer(host, portNum);

    // Store room and name for use in onConnected()
    m_roomEdit->setText(room);
    m_nameEdit->setText(name);
}

void RoomWidget::onReadyClicked()
{
    m_localChar = static_cast<CharacterType>(m_charCombo->currentIndex());

    m_network->send(NM::build_ready(static_cast<int>(m_localChar)));
    setPhase(Phase::Waiting);
    setStatus("Waiting for opponent...", "#FFE066");
    m_readyBtn->setEnabled(false);
}

void RoomWidget::onBackClicked()
{
    m_network->shutdown();
    emit backRequested();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Network slots
// ─────────────────────────────────────────────────────────────────────────────

void RoomWidget::onConnected()
{
    QString room = m_roomEdit->text().trimmed().toUpper();
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) name = "Player";

    m_network->send(NM::build_join(room.toStdString(), name.toStdString()));
    setStatus("Connected. Joining room " + room + "...", "#FFE066");
}

void RoomWidget::onConnectionFailed(const QString& reason)
{
    setPhase(Phase::Disconnected);
    setStatus("Connection failed: " + reason, "#EE3333");
}

void RoomWidget::onDisconnected(const QString& reason)
{
    if (m_phase == Phase::Ready) return;   // game started, ignore
    setPhase(Phase::Disconnected);
    setStatus("Disconnected: " + reason, "#EE3333");
    m_p1Label->setText("P1: —");
    m_p2Label->setText("P2: —");
}

void RoomWidget::onMessageReceived(const QString& qmsg)
{
    std::string msg = qmsg.toStdString();
    std::string type = NM::extract_type(msg);

    if (type == NM::JOINED) {
        int role = NM::extract_int(msg, "role");
        m_isHost = (role == 0);
        QString roleStr = m_isHost ? "HOST" : "GUEST";
        m_roomLabel->setText("Room: " + m_roomEdit->text().toUpper()
                             + "  [" + roleStr + "]");
        setStatus("Waiting for second player...", "#FFE066");
        setPhase(Phase::InRoom);

        // Update our own player slot
        if (m_isHost) m_p1Label->setText("P1: " + m_nameEdit->text() + " (you)");
        else          m_p2Label->setText("P2: " + m_nameEdit->text() + " (you)");

    } else if (type == NM::ROOM_READY) {
        // Both players connected — now select characters
        setPhase(Phase::Selecting);
        setStatus("Both players connected! Select your character and press READY.", "#44FF88");

        if (m_isHost) m_p2Label->setText("P2: Opponent ✓");
        else          m_p1Label->setText("P1: Opponent ✓");

    } else if (type == NM::GAME_START) {
        int p1char = NM::extract_int(msg, "p1char");
        int p2char = NM::extract_int(msg, "p2char");

        // Host is always P1, guest is P2
        if (m_isHost) {
            m_localChar  = static_cast<CharacterType>(p1char);
            m_remoteChar = static_cast<CharacterType>(p2char);
        } else {
            m_localChar  = static_cast<CharacterType>(p2char);
            m_remoteChar = static_cast<CharacterType>(p1char);
        }

        setPhase(Phase::Ready);
        setStatus("Starting...", "#44FF88");
        emit gameReady();

    } else if (type == NM::ERROR_MSG) {
        std::string reason = NM::extract_string(msg, "reason");
        setStatus("Server error: " + QString::fromStdString(reason), "#EE3333");
        setPhase(Phase::Disconnected);

    } else if (type == NM::ROOM_CLOSED) {
        setPhase(Phase::Disconnected);
        setStatus("The other player disconnected.", "#EE3333");
        m_p1Label->setText("P1: —");
        m_p2Label->setText("P2: —");

    } else if (type == NM::PONG) {
        // keepalive reply — ignore silently
    }
}

void RoomWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    QPixmap bg(":/backgrounds/battle_bg.png");
    if (!bg.isNull()) p.drawPixmap(rect(), bg);
    else p.fillRect(rect(), QColor("#1A1A2E"));
    p.fillRect(rect(), QColor(0, 0, 0, 160));
}
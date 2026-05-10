#pragma once
#include <QWidget>
#include "networkmanager.h"
#include "character.h"

class QLabel;
class QPushButton;
class QLineEdit;
class QComboBox;
class QTimer;

// ─────────────────────────────────────────────────────────────────────────────
//  RoomWidget
//
//  Shown after "ONLINE PVP" is clicked. The player:
//    1. Enters server host + port (defaults: localhost:55000)
//    2. Enters a room code (create or join)
//    3. Selects their character
//    4. Waits for the second player to join
//    5. When both are ready, gameReady is emitted
// ─────────────────────────────────────────────────────────────────────────────
class RoomWidget : public QWidget {
    Q_OBJECT

public:
    explicit RoomWidget(QWidget* parent = nullptr);
    ~RoomWidget() override;

    // Call when this screen becomes visible
    void activate();
    void deactivate();

    // Getters for MainWindow to pass to OnlinePvpWidget
    CharacterType localCharacterType()  const { return m_localChar;  }
    CharacterType remoteCharacterType() const { return m_remoteChar; }
    bool          isHost()              const { return m_isHost;      }
    NetworkManager* network()                 { return m_network;     }

signals:
    void gameReady();        // both players ready, start the match
    void backRequested();    // player pressed back

protected:
    void paintEvent(QPaintEvent*) override;

private slots:
    void onConnectClicked();
    void onReadyClicked();
    void onBackClicked();
    void onConnected();
    void onConnectionFailed(const QString& reason);
    void onDisconnected(const QString& reason);
    void onMessageReceived(const QString& message);

private:
    void setStatus(const QString& text, const QString& color = "#F0E8D0");
    void buildUI();

    enum class Phase {
        Disconnected,   // initial state
        Connecting,     // waiting for TCP connection
        InRoom,         // connected, waiting for 2nd player
        Selecting,      // both in room, selecting characters
        Waiting,        // local ready, waiting for remote ready
        Ready           // both ready — emits gameReady()
    };

    void setPhase(Phase p);

    Phase          m_phase      = Phase::Disconnected;
    bool           m_isHost     = false;
    CharacterType  m_localChar  = CharacterType::Warrior;
    CharacterType  m_remoteChar = CharacterType::Warrior;

    NetworkManager* m_network   = nullptr;

    // UI elements
    QLineEdit*   m_hostEdit     = nullptr;
    QLineEdit*   m_portEdit     = nullptr;
    QLineEdit*   m_roomEdit     = nullptr;
    QLineEdit*   m_nameEdit     = nullptr;
    QComboBox*   m_charCombo    = nullptr;
    QPushButton* m_connectBtn   = nullptr;
    QPushButton* m_readyBtn     = nullptr;
    QPushButton* m_backBtn      = nullptr;
    QLabel*      m_statusLabel  = nullptr;
    QLabel*      m_roomLabel    = nullptr;
    QLabel*      m_p1Label      = nullptr;
    QLabel*      m_p2Label      = nullptr;
};
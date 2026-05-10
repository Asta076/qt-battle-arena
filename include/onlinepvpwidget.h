#pragma once

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QTimer>
#include <QSet>
#include <atomic>
#include <cstdint>

#include "character.h"
#include "networkmanager.h"
#include "networkmessage.h"
#include "goldhudwidget.h"
#include "healthbarwidget.h"
#include "dungeonwidget.h"   // DungeonPlayerSprite, DungeonSpriteSheet, DungeonAttackSheet
#include "worldcombatmanager.h"
#include "enemy.h"
#include "playercontroller.h"
#include "audiomanager.h"

class QLabel;
class QPushButton;

// ─────────────────────────────────────────────────────────────────────────────
//  OnlinePvpWidget
//
//  Real-time online PvP arena. Copied from PvpArenaWidget and extended with
//  networking. No game logic changes — only the input/state sync is new.
//
//  Authority model:
//    HOST   — runs the full simulation. Sends authoritative state every tick.
//    GUEST  — sends local inputs. Receives state from host and renders it.
//
//  Round agreement:
//    After a round ends, both players have 20 seconds to vote "next round".
//    If both vote yes → next round starts.
//    If either votes no OR timer expires → both returned to RoomWidget.
// ─────────────────────────────────────────────────────────────────────────────
class OnlinePvpWidget : public QWidget {
    Q_OBJECT

public:
    explicit OnlinePvpWidget(AudioManager* audio, QWidget* parent = nullptr);
    ~OnlinePvpWidget() override;

    // Call before showing. Network pointer is owned by RoomWidget — do not delete here.
    void activate(CharacterType localChar, CharacterType remoteChar,
                  bool isHost, NetworkManager* network);
    void deactivate();

signals:
    void returnToRoom();   // both players done — back to room widget

protected:
    void keyPressEvent  (QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
    void resizeEvent    (QResizeEvent* e) override;

private slots:
    void onTick();
    void onMessageReceived(const QString& message);
    void onDisconnected(const QString& reason);
    void onVoteTimerTick();

private:
    // ── Scene ─────────────────────────────────────────────────────────────────
    void buildScene(CharacterType localChar, CharacterType remoteChar);
    void buildHud();
    void updateHud();
    void placeSprites();
    void fitView();

    // ── Game loop ─────────────────────────────────────────────────────────────
    void processLocalInput();    // host: moves local sprite + sends state
                                 // guest: sends input keys, applies received state
    void sendInputToHost();      // guest only
    void sendStateToGuest();     // host only
    void applyReceivedState(const std::string& msg);
    void applyRemoteInput(uint32_t keyMask);

    uint32_t buildKeyMask() const;   // encode held keys into bitmask

    void checkCombat();              // host only — damage calculation
    void handleAttack(bool isLocal); // host only
    void handleSpecial(bool isLocal);

    void endRound(int winner);       // 1 = local won, 2 = remote won
    void showRoundOverlay(const QString& text);
    void startVotePhase();

    // ── State ─────────────────────────────────────────────────────────────────
    static constexpr qreal WORLD_W = 800;
    static constexpr qreal WORLD_H = 500;

    // Tick / network sequence number — monotonically increasing
    uint32_t m_seq = 0;

    bool m_isHost  = false;
    bool m_running = false;

    // Held keys for local player
    QSet<int> m_heldKeys;

    // Remote player's last known input (host only — decoded from INPUT messages)
    uint32_t m_remoteKeyMask = 0;

    // Attack flags — set by keypress, consumed by checkCombat
    bool m_localAttackPending   = false;
    bool m_localSpecialPending  = false;
    bool m_remoteAttackPending  = false;   // host only
    bool m_remoteSpecialPending = false;   // host only

    // Scores
    int m_localScore  = 0;
    int m_remoteScore = 0;

    // Round vote
    QTimer   m_voteTimer;
    int      m_voteCountdown = 20;   // seconds
    bool     m_localVoted    = false;
    bool     m_remoteVoted   = false;

    // ── Scene objects ─────────────────────────────────────────────────────────
    QGraphicsScene*       m_scene      = nullptr;
    QGraphicsView*        m_view       = nullptr;
    DungeonPlayerSprite*  m_localSprite  = nullptr;
    DungeonPlayerSprite*  m_remoteSprite = nullptr;
    DungeonSpriteSheet    m_localSheet;
    DungeonAttackSheet    m_localAtkSheet;
    DungeonSpriteSheet    m_remoteSheet;
    DungeonAttackSheet    m_remoteAtkSheet;
    Direction             m_localFacing  = Direction::Right;
    Direction             m_remoteFacing = Direction::Left;

    PlayerController m_controller;

    // ── Combat (host-authoritative) ───────────────────────────────────────────
    Character* m_localChar  = nullptr;
    Character* m_remoteChar = nullptr;

    // ── HUD ──────────────────────────────────────────────────────────────────
    QWidget*         m_hudWidget     = nullptr;
    HealthBarWidget* m_localHpBar    = nullptr;
    HealthBarWidget* m_remoteHpBar   = nullptr;
    QLabel*          m_localHpLabel  = nullptr;
    QLabel*          m_remoteHpLabel = nullptr;
    QLabel*          m_scoreLabel    = nullptr;
    QLabel*          m_roundOverlay  = nullptr;
    QLabel*          m_voteLabel     = nullptr;
    QPushButton*     m_nextRoundBtn  = nullptr;
    QPushButton*     m_leaveBtn      = nullptr;

    // ── Infrastructure ────────────────────────────────────────────────────────
    QTimer          m_ticker;
    NetworkManager* m_network  = nullptr;   // not owned
    AudioManager*   m_audio    = nullptr;
};
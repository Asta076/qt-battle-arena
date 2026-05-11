#pragma once
#include <QWidget>
#include "gameengine.h"
#include "battleresult.h"
#include "audiomanager.h"
#include "playerprofile.h"
#include "leveldef.h"

class HealthBarWidget;
class SpriteWidget;
class PauseOverlayWidget;
class QLabel;
class QPushButton;
class QKeyEvent;
class ItemMenuWidget;

// ─────────────────────────────────────────────────────────────────────────────
//  LevelBattleWidget
//
//  A completely separate battle screen used only for level fights (both
//  regular enemies and boss fights). Visually distinct from DungeonWidget's
//  battle screen — full-screen, theme-colored, cinematic layout.
//
//  Reuses GameEngine signals/slots but has its own layout and paintEvent.
// ─────────────────────────────────────────────────────────────────────────────
class LevelBattleWidget : public QWidget {
    Q_OBJECT

public:
    explicit LevelBattleWidget(GameEngine* engine,
                               AudioManager* audio,
                               PlayerProfile* profile,
                               QWidget* parent = nullptr);

    // Call before the fight starts to set context
    void setContext(const QString& levelName,
                    bool isBoss,
                    LevelTheme theme);

signals:
    void itemChosen(ItemType type);

protected:
    void paintEvent    (QPaintEvent*)  override;
    void resizeEvent   (QResizeEvent*) override;
    void keyPressEvent (QKeyEvent*)    override;

private slots:
    void onStateChanged         (GameState state);
    void onHealthUpdated        (float playerPct, float enemyPct);
    void onBattleActionResolved (const BattleResult& result);
    void onRoundEnded           (int pScore, int eScore, bool playerWon);
    void onEnergyUpdated        (float playerPct, float enemyPct);

private:
    void layoutChildren();
    void showMessage(const QString& msg);
    void onItemMenuChosen    (ItemType type);
    void onItemMenuCancelled ();
    void setButtonsEnabled(bool on);

    // ── Engine / data ─────────────────────────────────────────────────────────
    GameEngine*    m_engine;
    AudioManager*  m_audio;
    PlayerProfile* m_profile;

    // ── Context ───────────────────────────────────────────────────────────────
    QString    m_levelName;
    bool       m_isBoss  = false;
    LevelTheme m_theme   = LevelTheme::Cave;

    // ── Sprites ───────────────────────────────────────────────────────────────
    SpriteWidget* m_playerSprite = nullptr;
    SpriteWidget* m_enemySprite  = nullptr;

    // ── HUD labels ────────────────────────────────────────────────────────────
    QLabel* m_headerLabel     = nullptr;   // "LEVEL 2 — HAUNTED FOREST"
    QLabel* m_subheaderLabel  = nullptr;   // "⚔ BOSS BATTLE" or "ENCOUNTER"
    QLabel* m_playerNameLabel = nullptr;
    QLabel* m_enemyNameLabel  = nullptr;
    QLabel* m_messageLabel    = nullptr;   // single-line battle message
    QLabel* m_scoreLabel      = nullptr;   // "2 — 1" for boss multi-round

    // ── HP / SP bars ──────────────────────────────────────────────────────────
    HealthBarWidget* m_playerHP = nullptr;
    HealthBarWidget* m_enemyHP  = nullptr;
    HealthBarWidget* m_playerSP = nullptr;
    HealthBarWidget* m_enemySP  = nullptr;

    // ── Action buttons ────────────────────────────────────────────────────────
    QPushButton* m_fightBtn   = nullptr;
    QPushButton* m_specialBtn = nullptr;
    QPushButton* m_itemBtn    = nullptr;
    QPushButton* m_runBtn     = nullptr;

    int m_cursorIndex = 0;
    void moveCursor(int delta);
    void confirmSelection();
    void updateCursor();

    // ── Overlays ──────────────────────────────────────────────────────────────
    PauseOverlayWidget* m_pauseOverlay = nullptr;
    ItemMenuWidget*     m_itemMenu     = nullptr;

    // ── Theme helper ─────────────────────────────────────────────────────────
    QColor themeColor()       const;   // primary accent colour
    QColor themeBgColor()     const;   // background tint
    QColor themeOverlay()     const;   // semi-transparent overlay
};

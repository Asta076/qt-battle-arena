#pragma once
#include <QWidget>
#include "leveldef.h"

class QLabel;
class QPushButton;
class QTimer;

// ─────────────────────────────────────────────────────────────────────────────
//  BossDialogWidget
//
//  Full-screen overlay shown before and after a boss fight.
//  Typewriter effect reveals text character by character.
//  Player presses Space/Enter to advance or skip to the end.
//  Emits fightAccepted() when player confirms the fight.
//  Emits dialogDismissed() after a post-battle dialog is dismissed.
// ─────────────────────────────────────────────────────────────────────────────
class BossDialogWidget : public QWidget {
    Q_OBJECT
public:
    explicit BossDialogWidget(QWidget* parent = nullptr);

    // Call before showing for pre-fight dialog
    void showIntro(const LevelDef& level, const QString& playerName);

    // Call before showing for post-fight dialog
    void showOutro(const LevelDef& level, bool playerWon);

signals:
    void fightAccepted();     // player hit FIGHT after intro
    void dialogDismissed();   // player dismissed outro

protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void mousePressEvent(QMouseEvent*) override;

private slots:
    void onTypewriterTick();

private:
    enum class Mode { Intro, Outro };

    void startTypewriter(const QString& fullText);
    void finishTypewriter();
    void advance();           // space/enter/click handler

    Mode         m_mode       = Mode::Intro;
    bool         m_isIntro    = true;
    bool         m_typing     = false;   // typewriter in progress
    bool         m_waitingForFight = false;

    QString      m_fullText;
    QString      m_displayedText;
    int          m_charIndex  = 0;

    LevelDef     m_level;

    QLabel*      m_bossNameLabel  = nullptr;
    QLabel*      m_dialogLabel    = nullptr;
    QLabel*      m_hintLabel      = nullptr;  // "Press SPACE to continue"
    QPushButton* m_fightButton    = nullptr;
    QPushButton* m_dismissButton  = nullptr;
    QTimer*      m_typeTimer      = nullptr;

    static constexpr int TYPEWRITER_MS = 35;  // ms per character — adjust freely
};
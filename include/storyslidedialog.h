#pragma once
#include <QWidget>
#include <QStringList>

class QLabel;
class QPushButton;
class QTimer;

// ─────────────────────────────────────────────────────────────────────────────
//  StorySlideDialog
//
//  Full-screen overlay that displays paginated story text with a typewriter
//  effect before a level begins. Works the same as BossDialogWidget but for
//  narrative context rather than boss dialogue.
//
//  Usage:
//    m_storySlide->show("Chapter Title", {"Page 1", "Page 2", ...}, "► ENTER");
//    connect(m_storySlide, &StorySlideDialog::finished, this, &MainWindow::onStoryFinished);
//
//  Space / click skips typewriter or advances to next page.
//  The enter button only appears on the last page.
// ─────────────────────────────────────────────────────────────────────────────
class StorySlideDialog : public QWidget {
    Q_OBJECT
public:
    explicit StorySlideDialog(QWidget* parent = nullptr);

    // chapterTitle  — shown small at top e.g. "Chapter 2 — The Haunted Forest"
    // pages         — each string is one screen of story text
    // enterPrompt   — label on the final button e.g. "► ENTER THE FOREST"
    void show(const QString& chapterTitle,
              const QStringList& pages,
              const QString& enterPrompt = "► CONTINUE");

signals:
    void finished();   // emitted when player presses the enter button on last page

protected:
    void paintEvent    (QPaintEvent*)   override;
    void keyPressEvent (QKeyEvent*)     override;
    void mousePressEvent(QMouseEvent*)  override;

private slots:
    void onTypewriterTick();

private:
    void startPage      (int index);
    void startTypewriter(const QString& text);
    void finishTypewriter();
    void advance();        // space / click handler

    // ── Widgets ───────────────────────────────────────────────────────────────
    QLabel*      m_chapterLabel = nullptr;   // "— CHAPTER 2 — THE HAUNTED FOREST —"
    QLabel*      m_textLabel    = nullptr;   // main story body
    QLabel*      m_pageLabel    = nullptr;   // "2 / 5"
    QLabel*      m_hintLabel    = nullptr;   // "[ SPACE ] to continue"
    QPushButton* m_enterBtn     = nullptr;   // shown only on last page

    // ── Typewriter state ─────────────────────────────────────────────────────
    QTimer*     m_typeTimer     = nullptr;
    QStringList m_pages;
    int         m_currentPage   = 0;
    QString     m_fullText;
    QString     m_displayedText;
    int         m_charIndex     = 0;
    bool        m_typing        = false;

    static constexpr int TYPEWRITER_MS = 28;   // ms per character — adjust for pacing
};

#include "storyslidedialog.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QFont>

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────

StorySlideDialog::StorySlideDialog(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::StrongFocus);
    hide();

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(100, 70, 100, 70);
    root->setSpacing(0);

    root->addStretch(2);

    // ── Chapter title ─────────────────────────────────────────────────────────
    m_chapterLabel = new QLabel(this);
    m_chapterLabel->setAlignment(Qt::AlignCenter);
    m_chapterLabel->setFont(QFont("Press Start 2P", 7));
    m_chapterLabel->setStyleSheet("color: #A0A0C0; letter-spacing: 3px;");
    root->addWidget(m_chapterLabel);
    root->addSpacing(32);

    // ── Story text ────────────────────────────────────────────────────────────
    m_textLabel = new QLabel(this);
    m_textLabel->setWordWrap(true);
    m_textLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_textLabel->setFont(QFont("Press Start 2P", 8));
    m_textLabel->setStyleSheet("color: #F0E8D0; line-height: 2.0;");
    m_textLabel->setMinimumHeight(220);
    root->addWidget(m_textLabel);
    root->addSpacing(24);

    // ── Page indicator ────────────────────────────────────────────────────────
    m_pageLabel = new QLabel(this);
    m_pageLabel->setAlignment(Qt::AlignRight);
    m_pageLabel->setFont(QFont("Press Start 2P", 6));
    m_pageLabel->setStyleSheet("color: #3A3A5A;");
    root->addWidget(m_pageLabel);
    root->addSpacing(12);

    // ── Hint ──────────────────────────────────────────────────────────────────
    m_hintLabel = new QLabel("[ SPACE ] to continue", this);
    m_hintLabel->setAlignment(Qt::AlignCenter);
    m_hintLabel->setFont(QFont("Press Start 2P", 6));
    m_hintLabel->setStyleSheet("color: #555570;");
    root->addWidget(m_hintLabel);
    root->addSpacing(16);

    // ── Enter button — last page only ─────────────────────────────────────────
    m_enterBtn = new QPushButton("► CONTINUE", this);
    m_enterBtn->setFixedWidth(280);
    m_enterBtn->hide();
    connect(m_enterBtn, &QPushButton::clicked,
            this, &StorySlideDialog::finished);

    auto* btnRow = new QHBoxLayout;
    btnRow->setAlignment(Qt::AlignCenter);
    btnRow->addWidget(m_enterBtn);
    root->addLayout(btnRow);

    root->addStretch(1);

    // ── Typewriter timer ──────────────────────────────────────────────────────
    m_typeTimer = new QTimer(this);
    m_typeTimer->setInterval(TYPEWRITER_MS);
    connect(m_typeTimer, &QTimer::timeout,
            this, &StorySlideDialog::onTypewriterTick);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────────────────────────────────────

void StorySlideDialog::show(const QString& chapterTitle,
                            const QStringList& pages,
                            const QString& enterPrompt)
{
    if (pages.isEmpty()) {
        emit finished();
        return;
    }

    m_pages       = pages;
    m_currentPage = 0;
    m_enterBtn->setText(enterPrompt);

    m_chapterLabel->setText(
        QString("— %1 —").arg(chapterTitle.toUpper()));

    QWidget::show();
    raise();
    setFocus();
    startPage(0);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Page management
// ─────────────────────────────────────────────────────────────────────────────

void StorySlideDialog::startPage(int index)
{
    if (index >= m_pages.size()) {
        emit finished();
        QWidget::hide();
        return;
    }

    m_currentPage = index;

    m_pageLabel->setText(
        QString("%1 / %2").arg(index + 1).arg(m_pages.size()));

    m_enterBtn->hide();
    m_hintLabel->show();

    startTypewriter(m_pages[index]);
}

void StorySlideDialog::startTypewriter(const QString& text)
{
    m_fullText      = text;
    m_displayedText = "";
    m_charIndex     = 0;
    m_typing        = true;
    m_textLabel->setText("");
    m_typeTimer->start();
}

void StorySlideDialog::finishTypewriter()
{
    m_typeTimer->stop();
    m_typing        = false;
    m_displayedText = m_fullText;
    m_textLabel->setText(m_fullText);

    // On the last page show the enter button instead of the hint
    bool isLast = (m_currentPage >= m_pages.size() - 1);
    if (isLast) {
        m_hintLabel->hide();
        m_enterBtn->show();
        m_enterBtn->setFocus();
    }
}

void StorySlideDialog::onTypewriterTick()
{
    if (m_charIndex >= m_fullText.length()) {
        finishTypewriter();
        return;
    }
    m_displayedText += m_fullText[m_charIndex++];
    m_textLabel->setText(m_displayedText);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Input — space/click skips typewriter or advances page
// ─────────────────────────────────────────────────────────────────────────────

void StorySlideDialog::advance()
{
    if (m_typing) {
        // Skip to end of current page
        finishTypewriter();
        return;
    }

    // If not on last page, go forward
    bool isLast = (m_currentPage >= m_pages.size() - 1);
    if (!isLast) {
        startPage(m_currentPage + 1);
    }
    // Last page: the enter button handles the emit
}

void StorySlideDialog::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Space || e->key() == Qt::Key_Return)
        advance();
    else
        QWidget::keyPressEvent(e);
}

void StorySlideDialog::mousePressEvent(QMouseEvent*)
{
    advance();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Paint — cinematic dark overlay with gold accent lines
// ─────────────────────────────────────────────────────────────────────────────

void StorySlideDialog::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    // Full screen blackout
    p.fillRect(rect(), QColor(0, 0, 0, 235));

    const int margin = 60;
    const int w = width();
    const int h = height();

    // Outer gold frame
    p.setPen(QPen(QColor("#FFE066"), 1));
    p.drawLine(margin,     margin,     w - margin, margin);
    p.drawLine(margin,     h - margin, w - margin, h - margin);
    p.drawLine(margin,     margin,     margin,     h - margin);
    p.drawLine(w - margin, margin,     w - margin, h - margin);

    // Inner dark border (slightly inset)
    p.setPen(QPen(QColor("#3A3A5A"), 1));
    p.drawLine(margin + 6,     margin + 6,     w - margin - 6, margin + 6);
    p.drawLine(margin + 6,     h - margin - 6, w - margin - 6, h - margin - 6);
    p.drawLine(margin + 6,     margin + 6,     margin + 6,     h - margin - 6);
    p.drawLine(w - margin - 6, margin + 6,     w - margin - 6, h - margin - 6);

    // Corner accents
    const int cs = 16;   // corner size
    p.setPen(QPen(QColor("#FFE066"), 2));
    // top-left
    p.drawLine(margin, margin, margin + cs, margin);
    p.drawLine(margin, margin, margin, margin + cs);
    // top-right
    p.drawLine(w - margin - cs, margin, w - margin, margin);
    p.drawLine(w - margin, margin, w - margin, margin + cs);
    // bottom-left
    p.drawLine(margin, h - margin - cs, margin, h - margin);
    p.drawLine(margin, h - margin, margin + cs, h - margin);
    // bottom-right
    p.drawLine(w - margin - cs, h - margin, w - margin, h - margin);
    p.drawLine(w - margin, h - margin - cs, w - margin, h - margin);
}

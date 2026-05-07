#include "bossdialogwidget.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QKeyEvent>
#include <QTimer>
#include <QFont>

BossDialogWidget::BossDialogWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::StrongFocus);

    // ── Layout ────────────────────────────────────────────────────────────────
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(60, 40, 60, 40);
    root->setSpacing(0);

    root->addStretch(3);

    // Boss name banner
    m_bossNameLabel = new QLabel(this);
    m_bossNameLabel->setObjectName("titleLabel");
    m_bossNameLabel->setAlignment(Qt::AlignCenter);
    m_bossNameLabel->setStyleSheet("color: #FFD700; font-size: 13px;");
    root->addWidget(m_bossNameLabel);
    root->addSpacing(12);

    // Dialog text box
    auto* textBox = new QWidget(this);
    textBox->setObjectName("dialogBox");
    textBox->setFixedHeight(120);
    auto* textLayout = new QVBoxLayout(textBox);
    textLayout->setContentsMargins(20, 16, 20, 16);

    m_dialogLabel = new QLabel(textBox);
    m_dialogLabel->setWordWrap(true);
    m_dialogLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_dialogLabel->setStyleSheet("color: #F0E8D0; font-size: 9px;");
    m_dialogLabel->setFont(QFont("Press Start 2P", 9));
    textLayout->addWidget(m_dialogLabel);

    root->addWidget(textBox);
    root->addSpacing(8);

    // Hint label
    m_hintLabel = new QLabel("[ SPACE ] to continue", this);
    m_hintLabel->setAlignment(Qt::AlignCenter);
    m_hintLabel->setStyleSheet("color: #888880; font-size: 7px;");
    m_hintLabel->setFont(QFont("Press Start 2P", 7));
    root->addWidget(m_hintLabel);
    root->addSpacing(16);

    // Fight / Dismiss buttons
    auto* btnRow = new QHBoxLayout;
    btnRow->setAlignment(Qt::AlignCenter);
    btnRow->setSpacing(24);

    m_fightButton = new QPushButton("⚔  FIGHT!", this);
    m_fightButton->setFixedWidth(180);
    m_fightButton->hide();
    connect(m_fightButton, &QPushButton::clicked,
            this, &BossDialogWidget::fightAccepted);

    m_dismissButton = new QPushButton("← BACK", this);
    m_dismissButton->setFixedWidth(180);
    m_dismissButton->hide();
    connect(m_dismissButton, &QPushButton::clicked,
            this, &BossDialogWidget::dialogDismissed);

    btnRow->addWidget(m_fightButton);
    btnRow->addWidget(m_dismissButton);
    root->addLayout(btnRow);

    root->addStretch(1);

    // Typewriter timer
    m_typeTimer = new QTimer(this);
    m_typeTimer->setInterval(TYPEWRITER_MS);
    connect(m_typeTimer, &QTimer::timeout,
            this, &BossDialogWidget::onTypewriterTick);
}

// ─────────────────────────────────────────────────────────────────────────────

void BossDialogWidget::showIntro(const LevelDef& level, const QString& playerName)
{
    m_level  = level;
    m_mode   = Mode::Intro;

    m_bossNameLabel->setText(
        QString("— %1 —").arg(level.bossName.toUpper()));

    m_fightButton->hide();
    m_dismissButton->hide();
    m_hintLabel->show();
    m_waitingForFight = false;

    // Replace placeholder name with player name if present
    QString line = level.bossIntroLine;
    line.replace("{player}", playerName);
    startTypewriter(line);
}

void BossDialogWidget::showOutro(const LevelDef& level, bool playerWon)
{
    m_level = level;
    m_mode  = Mode::Outro;

    m_bossNameLabel->setText(
        QString("— %1 —").arg(level.bossName.toUpper()));

    m_fightButton->hide();
    m_dismissButton->hide();
    m_hintLabel->show();

    startTypewriter(playerWon ? level.bossDefeatLine : level.bossVictoryLine);
}

// ─────────────────────────────────────────────────────────────────────────────

void BossDialogWidget::startTypewriter(const QString& fullText)
{
    m_fullText      = fullText;
    m_displayedText = "";
    m_charIndex     = 0;
    m_typing        = true;
    m_dialogLabel->setText("");
    m_typeTimer->start();
}

void BossDialogWidget::finishTypewriter()
{
    m_typeTimer->stop();
    m_typing        = false;
    m_displayedText = m_fullText;
    m_dialogLabel->setText(m_fullText);

    m_hintLabel->hide();

    if (m_mode == Mode::Intro) {
        m_fightButton->show();
        m_fightButton->setFocus();
        m_waitingForFight = true;
    } else {
        m_dismissButton->show();
        m_dismissButton->setFocus();
    }
}

void BossDialogWidget::onTypewriterTick()
{
    if (m_charIndex >= m_fullText.length()) {
        finishTypewriter();
        return;
    }
    m_displayedText += m_fullText[m_charIndex++];
    m_dialogLabel->setText(m_displayedText);
}

// ─────────────────────────────────────────────────────────────────────────────

void BossDialogWidget::advance()
{
    if (m_typing) {
        // Skip to end of typewriter
        finishTypewriter();
    }
    // If not typing, buttons handle the next action — do nothing here
}

void BossDialogWidget::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Space || e->key() == Qt::Key_Return)
        advance();
    else
        QWidget::keyPressEvent(e);
}

void BossDialogWidget::mousePressEvent(QMouseEvent*)
{
    advance();
}

// ─────────────────────────────────────────────────────────────────────────────

void BossDialogWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);

    // Dark semi-transparent full-screen background
    p.fillRect(rect(), QColor(0, 0, 0, 200));

    // Dialog box background — bottom third of screen
    const int boxH   = 200;
    const int boxY   = height() - boxH - 60;
    const int boxX   = 40;
    const int boxW   = width() - 80;

    p.setPen(QPen(QColor("#FFD700"), 3));
    p.setBrush(QColor(13, 13, 26, 230));
    p.drawRect(boxX, boxY, boxW, boxH);

    // Inner border
    p.setPen(QPen(QColor("#F0E8D0"), 1));
    p.drawRect(boxX + 4, boxY + 4, boxW - 8, boxH - 8);
}
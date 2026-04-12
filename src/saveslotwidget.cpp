#include "saveslotwidget.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QMessageBox>

static QString classNameFromInt(int t)
{
    switch (t) {
    case 0:  return "WARRIOR";
    case 1:  return "MAGE";
    case 2:  return "ARCHER";
    default: return "???";
    }
}

QString SaveSlotWidget::slotPath(int index)
{
    return QString("save_slot_%1.json").arg(index);
}

SaveSlotWidget::SaveSlotWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setAlignment(Qt::AlignCenter);
    root->setSpacing(16);

    auto* title = new QLabel("SELECT SAVE SLOT", this);
    title->setObjectName("titleLabel");
    title->setAlignment(Qt::AlignCenter);

    auto* cardsRow = new QHBoxLayout;
    cardsRow->setSpacing(24);
    cardsRow->setAlignment(Qt::AlignCenter);

    for (int i = 0; i < 3; ++i) {
        auto* card = new QWidget(this);
        card->setObjectName("characterCard");
        card->setFixedSize(220, 260);

        auto* layout = new QVBoxLayout(card);
        layout->setAlignment(Qt::AlignCenter);
        layout->setSpacing(8);
        layout->setContentsMargins(12, 12, 12, 12);

        auto* slotNum   = new QLabel(QString("— SLOT %1 —").arg(i + 1), card);
        auto* nameLabel = new QLabel("EMPTY", card);
        auto* classLabel= new QLabel("", card);
        auto* statsLabel= new QLabel("", card);
        auto* btn       = new QPushButton("NEW GAME", card);

        slotNum->setObjectName("subtitleLabel");
        nameLabel->setObjectName("subtitleLabel");
        classLabel->setObjectName("subtitleLabel");
        statsLabel->setObjectName("subtitleLabel");
        btn->setFixedWidth(180);

        for (auto* lbl : {slotNum, nameLabel, classLabel, statsLabel})
            lbl->setAlignment(Qt::AlignCenter);

        const int captured = i;
        connect(btn, &QPushButton::clicked, this,
                [this, captured]{ onSlotClicked(captured); });

        layout->addWidget(slotNum,    0, Qt::AlignCenter);
        layout->addSpacing(4);
        layout->addWidget(nameLabel,  0, Qt::AlignCenter);
        layout->addWidget(classLabel, 0, Qt::AlignCenter);
        layout->addWidget(statsLabel, 0, Qt::AlignCenter);
        layout->addStretch();
        layout->addWidget(btn,        0, Qt::AlignCenter);

        m_cards[i] = { card, nameLabel, classLabel, statsLabel, btn };
        cardsRow->addWidget(card);
    }

    auto* backBtn = new QPushButton("← BACK", this);
    backBtn->setFixedWidth(180);
    connect(backBtn, &QPushButton::clicked, this, &SaveSlotWidget::backRequested);

    root->addStretch(1);
    root->addWidget(title);
    root->addSpacing(16);
    root->addLayout(cardsRow);
    root->addSpacing(16);
    root->addWidget(backBtn, 0, Qt::AlignCenter);
    root->addStretch(1);
}

void SaveSlotWidget::refresh(SlotMode mode)
{
    m_mode = mode;
    for (int i = 0; i < 3; ++i) {
        PlayerProfile p;
        m_occupied[i] = p.loadFromFile(slotPath(i));
        if (m_occupied[i]) m_profiles[i] = p;
        updateCard(i);
    }
}

void SaveSlotWidget::updateCard(int i)
{
    if (!m_cards[i].widget) return;

    if (!m_occupied[i]) {
        m_cards[i].nameLabel->setText("EMPTY");
        m_cards[i].classLabel->setText("");
        m_cards[i].statsLabel->setText("");
        m_cards[i].actionBtn->setText("NEW GAME");
    } else {
        const PlayerProfile& p = m_profiles[i];
        m_cards[i].nameLabel->setText(p.characterName);
        m_cards[i].classLabel->setText(classNameFromInt(p.characterType));
        m_cards[i].statsLabel->setText(
            QString("Gold: %1  Runs: %2").arg(p.gold).arg(p.dungeonRuns));
        m_cards[i].actionBtn->setText(
            m_mode == SlotMode::NewGame ? "OVERWRITE" : "CONTINUE");
    }
}

void SaveSlotWidget::onSlotClicked(int index)
{
    if (!m_occupied[index]) {
        emit newGameRequested(index);
        return;
    }

    if (m_mode == SlotMode::NewGame) {
        auto reply = QMessageBox::warning(
            this,
            "Overwrite Save?",
            QString("Slot %1 already has progress.\n\n"
                    "This will permanently delete it. Continue?")
                .arg(index + 1),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (reply == QMessageBox::Yes)
            emit newGameRequested(index);
    } else {
        emit loadRequested(index);
    }
}

void SaveSlotWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    QPixmap bg(":/backgrounds/battle_bg.png");
    if (!bg.isNull()) p.drawPixmap(rect(), bg);
    else p.fillRect(rect(), QColor("#1A1A2E"));
    p.fillRect(rect(), QColor(0, 0, 0, 140));
}
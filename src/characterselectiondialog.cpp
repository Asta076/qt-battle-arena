#include "characterselectiondialog.h"
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QStyle>
#include <QKeyEvent>

CharacterSelectWidget::CharacterSelectWidget(GameEngine* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setFocusPolicy(Qt::StrongFocus);

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setAlignment(Qt::AlignCenter);
    root->setSpacing(16);

    QLabel* title = new QLabel("CHOOSE YOUR FIGHTER", this);
    title->setObjectName("titleLabel");
    title->setAlignment(Qt::AlignCenter);

    // ── Three character cards ──────────────────
    QHBoxLayout* cards = new QHBoxLayout;
    cards->setSpacing(24);
    cards->setAlignment(Qt::AlignCenter);
    cards->addWidget(makeCard(CharacterType::Warrior, "WARRIOR",
                              "HP: 150  ATK: 20", "Power Strike"));
    cards->addWidget(makeCard(CharacterType::Mage,    "MAGE",
                              "HP:  80  ATK: 35", "Arcane Storm"));
    cards->addWidget(makeCard(CharacterType::Archer,  "ARCHER",
                              "HP: 110  ATK: 25", "Double Shot"));

    // ── Name input row — centered ──────────────
    QWidget* nameWidget = new QWidget(this);
    QHBoxLayout* nameRow = new QHBoxLayout(nameWidget);
    nameRow->setAlignment(Qt::AlignCenter);
    nameRow->setSpacing(12);
    QLabel* nameLabel = new QLabel("YOUR NAME:", nameWidget);
    m_nameInput = new QLineEdit(nameWidget);
    m_nameInput->setPlaceholderText("Enter name...");
    m_nameInput->setMaxLength(12);
    m_nameInput->setFixedWidth(200);
    nameRow->addWidget(nameLabel);
    nameRow->addWidget(m_nameInput);

    // ── Hint label ─────────────────────────────
    QLabel* hint = new QLabel("← → to select   ENTER to confirm", this);
    hint->setObjectName("subtitleLabel");
    hint->setAlignment(Qt::AlignCenter);

    // ── Confirm button — centered ──────────────
    m_confirmBtn = new QPushButton("► ENTER ARENA", this);
    m_confirmBtn->setFixedWidth(280);
    connect(m_confirmBtn, &QPushButton::clicked,
            this, &CharacterSelectWidget::onConfirm);

    root->addStretch(1);
    root->addWidget(title);
    root->addSpacing(12);
    root->addLayout(cards);
    root->addSpacing(8);
    root->addWidget(nameWidget);
    root->addWidget(hint, 0, Qt::AlignCenter);
    root->addSpacing(8);
    root->addWidget(m_confirmBtn, 0, Qt::AlignCenter);
    root->addStretch(1);

    selectCard(CharacterType::Warrior);
}

QWidget* CharacterSelectWidget::makeCard(CharacterType type,
                                         const QString& name,
                                         const QString& stats,
                                         const QString& special)
{
    QWidget*     card   = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(6);
    card->setFixedSize(200, 290);
    card->setObjectName("characterCard");

    QLabel* sprite = new QLabel(card);
    sprite->setFixedSize(120, 120);
    sprite->setAlignment(Qt::AlignCenter);

    QString path = (type == CharacterType::Warrior) ? ":/sprites/warrior_front.png"
                   : (type == CharacterType::Mage)    ? ":/sprites/mage_front.png"
                                                    : ":/sprites/archer_front.png";
    QPixmap px(path);
    if (!px.isNull())
        sprite->setPixmap(px.scaled(120, 120, Qt::KeepAspectRatio,
                                    Qt::FastTransformation));
    else
        sprite->setText(name.left(1));

    QLabel* nameLabel    = new QLabel(name,         card);
    QLabel* statsLabel   = new QLabel(stats,        card);
    QLabel* specialLabel = new QLabel("✦ " + special, card);

    nameLabel->setAlignment(Qt::AlignCenter);
    statsLabel->setAlignment(Qt::AlignCenter);
    specialLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setObjectName("cardTitle");

    QPushButton* selectBtn = new QPushButton("SELECT", card);
    selectBtn->setFixedWidth(160);
    connect(selectBtn, &QPushButton::clicked,
            this, [this, type]{ selectCard(type); });

    if (type == CharacterType::Warrior) m_warriorBtn = selectBtn;
    else if (type == CharacterType::Mage)   m_mageBtn = selectBtn;
    else                                  m_archerBtn = selectBtn;

    layout->addWidget(sprite,      0, Qt::AlignCenter);
    layout->addWidget(nameLabel,   0, Qt::AlignCenter);
    layout->addWidget(statsLabel,  0, Qt::AlignCenter);
    layout->addWidget(specialLabel,0, Qt::AlignCenter);
    layout->addStretch();
    layout->addWidget(selectBtn,   0, Qt::AlignCenter);

    return card;
}

void CharacterSelectWidget::selectCard(CharacterType type)
{
    m_selected = type;

    auto apply = [](QPushButton* btn, bool selected) {
        btn->setProperty("selected", selected);
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
        btn->update();
    };

    apply(m_warriorBtn, type == CharacterType::Warrior);
    apply(m_mageBtn,    type == CharacterType::Mage);
    apply(m_archerBtn,  type == CharacterType::Archer);
}

void CharacterSelectWidget::keyPressEvent(QKeyEvent* event)
{
    // Cycle through characters with arrow keys
    switch (event->key()) {
    case Qt::Key_Left:
        if      (m_selected == CharacterType::Mage)   selectCard(CharacterType::Warrior);
        else if (m_selected == CharacterType::Archer)  selectCard(CharacterType::Mage);
        break;
    case Qt::Key_Right:
        if      (m_selected == CharacterType::Warrior) selectCard(CharacterType::Mage);
        else if (m_selected == CharacterType::Mage)    selectCard(CharacterType::Archer);
        break;
    case Qt::Key_Return:
    case Qt::Key_Space:
        onConfirm();
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

void CharacterSelectWidget::onConfirm()
{
    QString name = m_nameInput->text().trimmed();
    if (name.isEmpty()) {
        name = (m_selected == CharacterType::Warrior) ? "Warrior"
               : (m_selected == CharacterType::Mage)    ? "Mage"
                                                      : "Archer";
    }
    m_engine->onPlayerSelectedCharacter(m_selected, name);
}

void CharacterSelectWidget::paintEvent(QPaintEvent* event)
{
    QPainter p(this);
    QPixmap bg(":/backgrounds/battle_bg.png");
    if (!bg.isNull()) p.drawPixmap(rect(), bg);
    else p.fillRect(rect(), QColor("#1A1A2E"));
    p.fillRect(rect(), QColor(0, 0, 0, 140));
    QWidget::paintEvent(event);
}

#include "scoreboardwidget.h"
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QPainter>

ScoreboardWidget::ScoreboardWidget(GameEngine* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(16);

    QLabel* title = new QLabel("FINAL SCOREBOARD", this);
    title->setObjectName("titleLabel");
    title->setAlignment(Qt::AlignCenter);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({"NAME", "TYPE", "HP LEFT", "STATUS"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setMinimumSize(600, 200);

    QPushButton* menuBtn = new QPushButton("► MAIN MENU", this);
    connect(menuBtn, &QPushButton::clicked, engine, &GameEngine::onExitToMenu);

    connect(engine, &GameEngine::stateChanged,
            this,   &ScoreboardWidget::onStateChanged);

    layout->addStretch();
    layout->addWidget(title);
    layout->addWidget(m_table);
    layout->addWidget(menuBtn, 0, Qt::AlignCenter);
    layout->addStretch();
}

void ScoreboardWidget::onStateChanged(GameState state)
{
    if (state == GameState::Scoreboard) refresh();
}

void ScoreboardWidget::refresh()
{
    const auto& chars = m_engine->getAllCharacters();
    m_table->setRowCount(chars.size());

    auto typeName = [](CharacterType t) -> QString {
        switch(t) {
        case CharacterType::Warrior: return "WARRIOR";
        case CharacterType::Mage:    return "MAGE";
        case CharacterType::Archer:  return "ARCHER";
        }
        return "?";
    };

    for (int i = 0; i < chars.size(); ++i) {
        auto* c = chars[i];
        m_table->setItem(i, 0, new QTableWidgetItem(c->getName()));
        m_table->setItem(i, 1, new QTableWidgetItem(typeName(c->getType())));
        m_table->setItem(i, 2, new QTableWidgetItem(
                                   QString::number(static_cast<int>(c->getHealthPercent() * c->getMaxHealth()))
                                   + " / " + QString::number(c->getMaxHealth())));

        QTableWidgetItem* status = new QTableWidgetItem(
            c->isAlive() ? "ALIVE" : "DEFEATED");
        status->setForeground(c->isAlive() ? QColor("#44BB44") : QColor("#EE3333"));
        m_table->setItem(i, 3, status);
    }
}

void ScoreboardWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    QPixmap bg(":/backgrounds/battle_bg.png");
    if (!bg.isNull()) p.drawPixmap(rect(), bg);
    else p.fillRect(rect(), QColor("#1A1A2E"));
    p.fillRect(rect(), QColor(0, 0, 0, 150));
}

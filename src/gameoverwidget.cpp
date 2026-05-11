#include "gameoverwidget.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "gameengine.h"

GameOverWidget::GameOverWidget(GameEngine* engine, QWidget* parent)
    : QWidget(parent)
    , m_engine(engine)
{
    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(18);

    m_resultLabel = new QLabel("✦ GAME OVER ✦", this);
    m_resultLabel->setObjectName("titleLabel");
    m_resultLabel->setAlignment(Qt::AlignCenter);

    m_scoreLabel = new QLabel("You were defeated in the dungeon.", this);
    m_scoreLabel->setObjectName("subtitleLabel");
    m_scoreLabel->setAlignment(Qt::AlignCenter);

    m_resultsTable = new QTableWidget(2, 2, this);
    m_resultsTable->setFixedSize(420, 150);
    m_resultsTable->setHorizontalHeaderLabels({"STAT", "VALUE"});
    m_resultsTable->verticalHeader()->setVisible(false);
    m_resultsTable->horizontalHeader()->setStretchLastSection(true);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultsTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_resultsTable->setFocusPolicy(Qt::NoFocus);

    auto* overworldBtn = new QPushButton("► RETURN TO OVERWORLD", this);
    auto* mainMenuBtn = new QPushButton("  MAIN MENU", this);

    connect(overworldBtn, &QPushButton::clicked, this, [this] {
        emit returnToOverworld();
    });

    connect(mainMenuBtn, &QPushButton::clicked, this, [this] {
        emit backToMenuRequested();
    });

    layout->addStretch();
    layout->addWidget(m_resultLabel, 0, Qt::AlignCenter);
    layout->addWidget(m_scoreLabel, 0, Qt::AlignCenter);
    layout->addSpacing(12);
    layout->addWidget(m_resultsTable, 0, Qt::AlignCenter);
    layout->addSpacing(18);
    layout->addWidget(overworldBtn, 0, Qt::AlignCenter);
    layout->addWidget(mainMenuBtn, 0, Qt::AlignCenter);
    layout->addStretch();

    showDungeonResults(0, 0);
}

void GameOverWidget::showDungeonResults(int coinsEarned, int wavesSurvived)
{
    m_resultLabel->setText("✦ GAME OVER ✦");
    m_scoreLabel->setText("You were defeated in the dungeon.");

    m_resultsTable->clearContents();

    m_resultsTable->setItem(0, 0, new QTableWidgetItem("Coins Earned"));
    m_resultsTable->setItem(0, 1, new QTableWidgetItem(QString::number(coinsEarned)));

    m_resultsTable->setItem(1, 0, new QTableWidgetItem("Waves Survived"));
    m_resultsTable->setItem(1, 1, new QTableWidgetItem(QString::number(wavesSurvived)));

    for (int row = 0; row < m_resultsTable->rowCount(); ++row) {
        for (int col = 0; col < m_resultsTable->columnCount(); ++col) {
            QTableWidgetItem* item = m_resultsTable->item(row, col);

            if (item)
                item->setTextAlignment(Qt::AlignCenter);
        }
    }
}

void GameOverWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter p(this);

    QPixmap bg(":/backgrounds/dungeon_bg.png");

    if (!bg.isNull())
        p.drawPixmap(rect(), bg);
    else
        p.fillRect(rect(), QColor("#1A1A2E"));

    p.fillRect(rect(), QColor(0, 0, 0, 170));
}

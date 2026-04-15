#include "shopwidget.h"
#include "ui_shopwidget.h"

#include <QPushButton>
#include <QListWidget>
#include <QString>

ShopWidget::ShopWidget(QWidget* parent)
    : QWidget(parent),
      ui(new Ui::ShopWidget)
{
    ui->setupUi(this);

    ui->itemListWidget->addItem("Health Potion - 30 gold");
    ui->itemListWidget->addItem("SP Potion - 25 gold");
    ui->itemListWidget->addItem("Attack Boost - 50 gold");
    ui->itemListWidget->addItem("Defense Boost - 50 gold");

    ui->descriptionLabel->setText("Select an item to see details.");

    connect(ui->buyButton, &QPushButton::clicked,
            this, &ShopWidget::onBuyClicked);

    connect(ui->backButton, &QPushButton::clicked,
            this, &ShopWidget::onBackClicked);

    connect(ui->itemListWidget, &QListWidget::currentRowChanged,
            this, &ShopWidget::onSelectionChanged);
}

ShopWidget::~ShopWidget()
{
    delete ui;
}

void ShopWidget::setGold(int gold)
{
    ui->goldLabel->setText(QString("Gold: %1").arg(gold));
}

void ShopWidget::onBuyClicked()
{
    emit buyItemRequested(m_selectedItem);
}

void ShopWidget::onBackClicked()
{
    emit backToOverworld();
}

void ShopWidget::onSelectionChanged(int row)
{
    switch (row) {
    case 0:
        m_selectedItem = ItemType::HealthPotion;
        ui->descriptionLabel->setText("Health Potion: restores 35% HP.");
        break;
    case 1:
        m_selectedItem = ItemType::SpPotion;
        ui->descriptionLabel->setText("SP Potion: restores 50 SP.");
        break;
    case 2:
        m_selectedItem = ItemType::AttackBoost;
        ui->descriptionLabel->setText("Attack Boost: increases attack.");
        break;
    case 3:
        m_selectedItem = ItemType::DefenseBoost;
        ui->descriptionLabel->setText("Defense Boost: reduces the next hit.");
        break;
    default:
        ui->descriptionLabel->setText("Select an item to see details.");
        break;
    }
}

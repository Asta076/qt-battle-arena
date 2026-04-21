#include "shopwidget.h"
#include "ui_shopwidget.h"
#include <QPushButton>
#include <QListWidget>
#include <QString>

// ── Helpers ───────────────────────────────────────────────────────────────────

static QString itemDisplayName(ItemType t)
{
    switch (t) {
    case ItemType::HealthPotion:  return "Health Potion";
    case ItemType::SpPotion:      return "SP Potion";
    case ItemType::AttackBoost:   return "Attack Boost";
    case ItemType::DefenseBoost:  return "Defense Boost";
    }
    return "Unknown";
}

// ─────────────────────────────────────────────────────────────────────────────

ShopWidget::ShopWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ShopWidget)
{
    ui->setupUi(this);

    // Populate list with name + price
    ui->itemListWidget->addItem(
        QString("Health Potion     — %1 G").arg(PRICE_HEALTH_POTION));
    ui->itemListWidget->addItem(
        QString("SP Potion         — %1 G").arg(PRICE_SP_POTION));
    ui->itemListWidget->addItem(
        QString("Attack Boost      — %1 G").arg(PRICE_ATTACK_BOOST));
    ui->itemListWidget->addItem(
        QString("Defense Boost     — %1 G").arg(PRICE_DEFENSE_BOOST));

    ui->itemListWidget->setCurrentRow(0);
    ui->descriptionLabel->setText(
        "Health Potion: restores 35% HP in battle.");

    connect(ui->buyButton,      &QPushButton::clicked,
            this, &ShopWidget::onBuyClicked);
    connect(ui->backButton,     &QPushButton::clicked,
            this, &ShopWidget::onBackClicked);
    connect(ui->itemListWidget, &QListWidget::currentRowChanged,
            this, &ShopWidget::onSelectionChanged);
}

ShopWidget::~ShopWidget()
{
    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────

void ShopWidget::setProfile(PlayerProfile* profile)
{
    m_profile = profile;
    refreshUI();
}

void ShopWidget::setGold(int gold)
{
    // Called by MainWindow — profile is the source of truth, but
    // we still update the label immediately so it reflects current gold.
    if (m_profile) m_profile->gold = gold;   // keep in sync
    ui->goldLabel->setText(QString("Gold: %1 G").arg(gold));
    refreshUI();
}

int ShopWidget::currentPrice() const
{
    switch (m_selectedItem) {
    case ItemType::HealthPotion:  return PRICE_HEALTH_POTION;
    case ItemType::SpPotion:      return PRICE_SP_POTION;
    case ItemType::AttackBoost:   return PRICE_ATTACK_BOOST;
    case ItemType::DefenseBoost:  return PRICE_DEFENSE_BOOST;
    }
    return 9999;
}

void ShopWidget::refreshUI()
{
    // Update gold label
    int gold = m_profile ? m_profile->gold : 0;
    ui->goldLabel->setText(QString("Gold: %1 G").arg(gold));

    // Show how many the player already owns in the description
    int owned = m_profile ? m_profile->itemCount(m_selectedItem) : 0;
    int price = currentPrice();
    bool canAfford = gold >= price;

    // Grey out the buy button if the player can't afford it
    ui->buyButton->setEnabled(canAfford);
    ui->buyButton->setText(
        canAfford
            ? QString("BUY  (%1 G)").arg(price)
            : QString("NEED %1 MORE G").arg(price - gold));

    // Update description with owned count
    QString desc;
    switch (m_selectedItem) {
    case ItemType::HealthPotion:
        desc = "Health Potion: restores 35% HP in battle.";
        break;
    case ItemType::SpPotion:
        desc = "SP Potion: restores 50 SP in battle.";
        break;
    case ItemType::AttackBoost:
        desc = "Attack Boost: +50% attack for one turn.";
        break;
    case ItemType::DefenseBoost:
        desc = "Defense Boost: blocks 30% of the next hit.";
        break;
    }
    desc += QString("  (Owned: %1)").arg(owned);
    ui->descriptionLabel->setText(desc);
}

// ─────────────────────────────────────────────────────────────────────────────

void ShopWidget::onBuyClicked()
{
    if (!m_profile) return;
    int price = currentPrice();
    if (m_profile->gold < price) return;   // double-guard

    emit buyItemRequested(m_selectedItem, price);
}

void ShopWidget::onBackClicked()
{
    emit backToOverworld();
}

void ShopWidget::onSelectionChanged(int row)
{
    switch (row) {
    case 0: m_selectedItem = ItemType::HealthPotion;  break;
    case 1: m_selectedItem = ItemType::SpPotion;      break;
    case 2: m_selectedItem = ItemType::AttackBoost;   break;
    case 3: m_selectedItem = ItemType::DefenseBoost;  break;
    default: return;
    }
    refreshUI();
}

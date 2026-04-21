#pragma once

#include <QWidget>
#include "playerprofile.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ShopWidget; }
QT_END_NAMESPACE

class ShopWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ShopWidget(QWidget* parent = nullptr);
    ~ShopWidget();

    void setGold(int gold);
    void setProfile(PlayerProfile* profile);

signals:
    void buyItemRequested(ItemType type, int cost);
    void backToOverworld();

private slots:
    void onBuyClicked();
    void onBackClicked();
    void onSelectionChanged(int row);

private:
    void refreshUI();

    Ui::ShopWidget*  ui = nullptr;
    PlayerProfile*   m_profile = nullptr;
    ItemType         m_selectedItem = ItemType::HealthPotion;

    // ── Item price table — change freely ─────────────────────────────────────
    static constexpr int PRICE_HEALTH_POTION  = 30;
    static constexpr int PRICE_SP_POTION      = 25;
    static constexpr int PRICE_ATTACK_BOOST   = 50;
    static constexpr int PRICE_DEFENSE_BOOST  = 50;

    int currentPrice() const;
};

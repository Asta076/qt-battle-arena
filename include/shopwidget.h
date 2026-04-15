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

signals:
    void buyItemRequested(ItemType type);
    void backToOverworld();

private slots:
    void onBuyClicked();
    void onBackClicked();
    void onSelectionChanged(int row);

private:
    Ui::ShopWidget* ui = nullptr;
    ItemType m_selectedItem = ItemType::HealthPotion;
};

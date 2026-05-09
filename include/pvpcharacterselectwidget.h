#pragma once

#include <QPixmap>
#include <QWidget>

#include "character.h"

class QComboBox;
class QPushButton;
class QPaintEvent;

class PvpCharacterSelectWidget : public QWidget {
    Q_OBJECT

public:
    explicit PvpCharacterSelectWidget(QWidget* parent = nullptr);

signals:
    void duelStartRequested(CharacterType p1Type,
                            CharacterType p2Type,
                            int roundsToWin);
    void backRequested();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    CharacterType selectedTypeFromCombo(QComboBox* combo) const;

    QPixmap m_background;

    QComboBox* m_p1Combo = nullptr;
    QComboBox* m_p2Combo = nullptr;
    QComboBox* m_matchTypeCombo = nullptr;

    QPushButton* m_startBtn = nullptr;
    QPushButton* m_backBtn = nullptr;
};
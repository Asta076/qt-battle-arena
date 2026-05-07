#pragma once

#include <QWidget>

#include "character.h"

class QComboBox;
class QPushButton;

class PvpCharacterSelectWidget : public QWidget {
    Q_OBJECT

public:
    explicit PvpCharacterSelectWidget(QWidget* parent = nullptr);

signals:
    void duelStartRequested(CharacterType p1Type, CharacterType p2Type);
    void backRequested();

private:
    CharacterType selectedTypeFromCombo(QComboBox* combo) const;

    QComboBox* m_p1Combo = nullptr;
    QComboBox* m_p2Combo = nullptr;

    QPushButton* m_startBtn = nullptr;
    QPushButton* m_backBtn = nullptr;
};

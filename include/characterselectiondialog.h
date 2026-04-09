#pragma once
#include <QWidget>
#include "gameengine.h"

class QPushButton;
class QLineEdit;
class QLabel;
class QKeyEvent;    // ← ADD

class CharacterSelectWidget : public QWidget {
    Q_OBJECT
public:
    explicit CharacterSelectWidget(GameEngine* engine, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent* event) override;   // ← ADD

private slots:
    void onConfirm();

private:
    QWidget*     makeCard(CharacterType type,
                      const QString& name,
                      const QString& stats,
                      const QString& special);

    GameEngine*      m_engine;
    QLineEdit*       m_nameInput;
    QPushButton*     m_warriorBtn;
    QPushButton*     m_mageBtn;
    QPushButton*     m_archerBtn;
    QPushButton*     m_confirmBtn;
    CharacterType    m_selected = CharacterType::Warrior;

    void selectCard(CharacterType type);
};

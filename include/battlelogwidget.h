#pragma once
#include <QPlainTextEdit>

class BattleLogWidget : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit BattleLogWidget(QWidget* parent = nullptr);

public slots:
    void appendMessage(const QString& msg);
};

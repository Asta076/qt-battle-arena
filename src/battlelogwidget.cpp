#include "battlelogwidget.h"
#include <QScrollBar>           // ← ADD THIS

BattleLogWidget::BattleLogWidget(QWidget* parent)
    : QPlainTextEdit(parent)
{
    setReadOnly(true);
    setMaximumBlockCount(50);     // cap the log at 50 lines
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setObjectName("battleLog");
}

void BattleLogWidget::appendMessage(const QString& msg)
{
    appendPlainText("▶ " + msg);
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

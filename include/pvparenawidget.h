#pragma once

#include <QPixmap>
#include <QPointF>
#include <QWidget>

#include "character.h"

class QPaintEvent;
class QKeyEvent;
class QPainter;

class PvpArenaWidget : public QWidget {
    Q_OBJECT

public:
    explicit PvpArenaWidget(QWidget* parent = nullptr);

    void activate();

    // Later the PvP character select will call this.
    void setFighters(CharacterType p1Type, CharacterType p2Type);

signals:
    void backToMenu();

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void resetPlayers();

    void drawPlayer(QPainter& painter,
                    const QPointF& pos,
                    const QPixmap& sheet,
                    int idleColumn,
                    const QString& label,
                    const QColor& outlineColor);

    QPixmap cropIdleFrame(const QPixmap& sheet, int idleColumn) const;
    QString movementSheetFor(CharacterType type) const;

    static constexpr qreal WORLD_W = 960.0;
    static constexpr qreal WORLD_H = 720.0;

    static constexpr qreal PLAYER_W = 72.0;
    static constexpr qreal PLAYER_H = 88.0;

    static constexpr int FRAME_W = 68;
    static constexpr int FRAME_H = 68;

    QPixmap m_arenaBackground;

    QPixmap m_p1Sheet;
    QPixmap m_p2Sheet;

    QPointF m_p1Pos;
    QPointF m_p2Pos;
};

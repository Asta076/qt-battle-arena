#pragma once

#include <QPixmap>
#include <QPointF>
#include <QWidget>

class QPaintEvent;
class QKeyEvent;
class QPainter;

class PvpArenaWidget : public QWidget {
    Q_OBJECT

public:
    explicit PvpArenaWidget(QWidget* parent = nullptr);

    void activate();

signals:
    void backToMenu();

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void resetPlayers();
    void drawPlayer(QPainter& painter,
                    const QPointF& pos,
                    const QString& label,
                    const QColor& outlineColor);

    static constexpr qreal WORLD_W = 960.0;
    static constexpr qreal WORLD_H = 720.0;
    static constexpr qreal PLAYER_W = 52.0;
    static constexpr qreal PLAYER_H = 70.0;

    QPixmap m_arenaBackground;
    QPixmap m_playerSprite;

    QPointF m_p1Pos;
    QPointF m_p2Pos;
};

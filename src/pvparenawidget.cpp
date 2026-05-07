#include "pvparenawidget.h"

#include <QColor>
#include <QFont>
#include <QKeyEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QString>
#include <QtMath>

PvpArenaWidget::PvpArenaWidget(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);

    m_arenaBackground.load(":/backgrounds/pvp_arena.png");

    setFighters(CharacterType::Warrior, CharacterType::Archer);
    resetPlayers();

    m_ticker.setInterval(16);
    connect(&m_ticker, &QTimer::timeout,
            this, &PvpArenaWidget::onTick);
}

void PvpArenaWidget::activate()
{
    resetPlayers();
    m_heldKeys.clear();

    setFocus(Qt::OtherFocusReason);
    m_ticker.start();
    update();
}

void PvpArenaWidget::deactivate()
{
    m_ticker.stop();
    m_heldKeys.clear();
}

QString PvpArenaWidget::movementSheetFor(CharacterType type) const
{
    switch (type) {
    case CharacterType::Warrior:
        return ":/sprites/warrior_movement_8dir_6frames.png";
    case CharacterType::Mage:
        return ":/sprites/mage_movement_8dir_6frames.png";
    case CharacterType::Archer:
        return ":/sprites/archer_movement_8dir_6frames.png";
    }

    return ":/sprites/archer_movement_8dir_6frames.png";
}

void PvpArenaWidget::setFighters(CharacterType p1Type, CharacterType p2Type)
{
    m_p1.sheet.load(movementSheetFor(p1Type));
    m_p2.sheet.load(movementSheetFor(p2Type));

    update();
}

void PvpArenaWidget::resetPlayers()
{
    m_p1.pos = QPointF(330.0, 430.0);
    m_p2.pos = QPointF(580.0, 430.0);

    m_p1.facing = PvpDirection::Right;
    m_p2.facing = PvpDirection::Left;

    m_p1.frameIndex = 0;
    m_p2.frameIndex = 0;

    m_p1.tickAccum = 0;
    m_p2.tickAccum = 0;
}

QPointF PvpArenaWidget::velocityForPlayer1() const
{
    qreal dx = 0.0;
    qreal dy = 0.0;

    if (m_heldKeys.contains(Qt::Key_W)) dy -= SPEED;
    if (m_heldKeys.contains(Qt::Key_S)) dy += SPEED;
    if (m_heldKeys.contains(Qt::Key_A)) dx -= SPEED;
    if (m_heldKeys.contains(Qt::Key_D)) dx += SPEED;

    if (dx != 0.0 && dy != 0.0) {
        dx *= 0.7071;
        dy *= 0.7071;
    }

    return QPointF(dx, dy);
}

QPointF PvpArenaWidget::velocityForPlayer2() const
{
    qreal dx = 0.0;
    qreal dy = 0.0;

    if (m_heldKeys.contains(Qt::Key_Up)) dy -= SPEED;
    if (m_heldKeys.contains(Qt::Key_Down)) dy += SPEED;
    if (m_heldKeys.contains(Qt::Key_Left)) dx -= SPEED;
    if (m_heldKeys.contains(Qt::Key_Right)) dx += SPEED;

    if (dx != 0.0 && dy != 0.0) {
        dx *= 0.7071;
        dy *= 0.7071;
    }

    return QPointF(dx, dy);
}

QPointF PvpArenaWidget::clampToArena(const QPointF& pos) const
{
    // These bounds keep players mostly inside the sandy arena area.
    const qreal minX = 170.0;
    const qreal maxX = WORLD_W - 170.0 - PLAYER_W;
    const qreal minY = 210.0;
    const qreal maxY = WORLD_H - 120.0 - PLAYER_H;

    return QPointF(
        qBound(minX, pos.x(), maxX),
        qBound(minY, pos.y(), maxY)
        );
}

PvpDirection PvpArenaWidget::directionFromVelocity(const QPointF& velocity,
                                                   PvpDirection fallback) const
{
    const qreal dx = velocity.x();
    const qreal dy = velocity.y();

    if (dx == 0.0 && dy == 0.0)
        return fallback;

    if (dx > 0.0 && dy < 0.0) return PvpDirection::UpRight;
    if (dx < 0.0 && dy < 0.0) return PvpDirection::UpLeft;
    if (dx > 0.0 && dy > 0.0) return PvpDirection::DownRight;
    if (dx < 0.0 && dy > 0.0) return PvpDirection::DownLeft;

    if (dx > 0.0) return PvpDirection::Right;
    if (dx < 0.0) return PvpDirection::Left;
    if (dy < 0.0) return PvpDirection::Up;
    if (dy > 0.0) return PvpDirection::Down;

    return fallback;
}

void PvpArenaWidget::updateFighterAnimation(PvpFighterAnim& fighter, bool isMoving)
{
    if (!isMoving) {
        fighter.frameIndex = 0;
        fighter.tickAccum = 0;
        return;
    }

    ++fighter.tickAccum;

    if (fighter.tickAccum >= TICKS_PER_FRAME) {
        fighter.tickAccum = 0;
        fighter.frameIndex = (fighter.frameIndex + 1) % WALK_FRAMES;
    }
}

void PvpArenaWidget::onTick()
{
    const QPointF p1Velocity = velocityForPlayer1();
    const QPointF p2Velocity = velocityForPlayer2();

    const bool p1Moving = p1Velocity.x() != 0.0 || p1Velocity.y() != 0.0;
    const bool p2Moving = p2Velocity.x() != 0.0 || p2Velocity.y() != 0.0;

    m_p1.facing = directionFromVelocity(p1Velocity, m_p1.facing);
    m_p2.facing = directionFromVelocity(p2Velocity, m_p2.facing);

    m_p1.pos = clampToArena(m_p1.pos + p1Velocity);
    m_p2.pos = clampToArena(m_p2.pos + p2Velocity);

    updateFighterAnimation(m_p1, p1Moving);
    updateFighterAnimation(m_p2, p2Moving);

    update();
}

QPixmap PvpArenaWidget::cropFrame(const PvpFighterAnim& fighter, bool isMoving) const
{
    if (fighter.sheet.isNull())
        return QPixmap();

    const int dirIndex = static_cast<int>(fighter.facing);

    int row = 0;
    int col = dirIndex;

    if (isMoving) {
        row = dirIndex + 1;
        col = fighter.frameIndex;
    }

    return fighter.sheet.copy(col * FRAME_W, row * FRAME_H, FRAME_W, FRAME_H);
}

void PvpArenaWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.setRenderHint(QPainter::Antialiasing, false);

    painter.fillRect(rect(), QColor("#050816"));

    const qreal scale = qMin(width() / WORLD_W, height() / WORLD_H);
    const qreal xOffset = (width() - WORLD_W * scale) / 2.0;
    const qreal yOffset = (height() - WORLD_H * scale) / 2.0;

    painter.save();
    painter.translate(xOffset, yOffset);
    painter.scale(scale, scale);

    if (!m_arenaBackground.isNull()) {
        painter.drawPixmap(
            QRect(0, 0, static_cast<int>(WORLD_W), static_cast<int>(WORLD_H)),
            m_arenaBackground
            );
    } else {
        painter.fillRect(QRectF(0, 0, WORLD_W, WORLD_H), QColor("#111827"));

        painter.setPen(QPen(QColor("#FACC15"), 4));
        painter.setBrush(QColor("#1F2937"));
        painter.drawRoundedRect(QRectF(80, 80, WORLD_W - 160, WORLD_H - 160), 20, 20);
    }

    drawPlayer(painter, m_p1, "P1", QColor("#3A86FF"));
    drawPlayer(painter, m_p2, "P2", QColor("#FF4D6D"));

    painter.restore();

    QRect controlsRect(0, height() - 46, width(), 34);

    painter.fillRect(
        controlsRect.adjusted(0, -6, 0, 6),
        QColor(0, 0, 0, 150)
        );

    painter.setPen(QColor("#F9FAFB"));
    painter.setFont(QFont("Arial", 14, QFont::Bold));
    painter.drawText(
        controlsRect,
        Qt::AlignCenter,
        "P1: WASD    |    P2: Arrow Keys    |    ESC: Back"
        );
}

void PvpArenaWidget::drawPlayer(QPainter& painter,
                                const PvpFighterAnim& fighter,
                                const QString& label,
                                const QColor& outlineColor)
{
    QRectF body(fighter.pos.x(), fighter.pos.y(), PLAYER_W, PLAYER_H);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 90));
    painter.drawEllipse(QRectF(body.x() + 8, body.y() + PLAYER_H - 12, PLAYER_W - 16, 14));

    const bool isMoving = fighter.tickAccum != 0 || fighter.frameIndex != 0;
    QPixmap frame = cropFrame(fighter, isMoving);

    if (!frame.isNull()) {
        painter.drawPixmap(
            body.toRect(),
            frame.scaled(
                body.size().toSize(),
                Qt::KeepAspectRatio,
                Qt::FastTransformation
                )
            );
    } else {
        painter.setBrush(QColor("#E5E7EB"));
        painter.setPen(QPen(outlineColor, 3));
        painter.drawRoundedRect(body, 8, 8);
    }

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(outlineColor, 3));
    painter.drawRoundedRect(body, 6, 6);

    painter.setPen(QColor("#FFFFFF"));
    painter.setFont(QFont("Arial", 13, QFont::Bold));
    painter.drawText(
        QRectF(body.x() - 20, body.y() - 26, body.width() + 40, 22),
        Qt::AlignCenter,
        label
        );
}

void PvpArenaWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat()) {
        QWidget::keyPressEvent(event);
        return;
    }

    if (event->key() == Qt::Key_Escape) {
        deactivate();
        emit backToMenu();
        event->accept();
        return;
    }

    m_heldKeys.insert(event->key());
    event->accept();
}

void PvpArenaWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat()) {
        QWidget::keyReleaseEvent(event);
        return;
    }

    m_heldKeys.remove(event->key());
    event->accept();
}

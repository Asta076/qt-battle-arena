#include "pvparenawidget.h"

#include <QColor>
#include <QFont>
#include <QKeyEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QString>
#include <QtGlobal>
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
    m_projectiles.clear();
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

QString PvpArenaWidget::attackSheetFor(CharacterType type) const
{
    switch (type) {
    case CharacterType::Warrior:
        return ":/sprites/warrior_attack_8dir_7frames.png";
    case CharacterType::Mage:
        return ":/sprites/mage_attack_8dir_7frames.png";
    case CharacterType::Archer:
        return ":/sprites/archer_attack_8dir_7frames.png";
    }

    return ":/sprites/archer_attack_8dir_7frames.png";
}

void PvpArenaWidget::setFighters(CharacterType p1Type, CharacterType p2Type)
{
    m_p1.type = p1Type;
    m_p2.type = p2Type;

    m_p1.movementSheet.load(movementSheetFor(p1Type));
    m_p1.attackSheet.load(attackSheetFor(p1Type));

    m_p2.movementSheet.load(movementSheetFor(p2Type));
    m_p2.attackSheet.load(attackSheetFor(p2Type));

    update();
}

void PvpArenaWidget::resetPlayers()
{
    m_p1.pos = QPointF(330.0, 430.0);
    m_p2.pos = QPointF(580.0, 430.0);

    m_p1.facing = PvpDirection::Right;
    m_p2.facing = PvpDirection::Left;

    m_p1.isMoving = false;
    m_p2.isMoving = false;

    m_p1.frameIndex = 0;
    m_p2.frameIndex = 0;

    m_p1.tickAccum = 0;
    m_p2.tickAccum = 0;

    m_p1.isAttacking = false;
    m_p2.isAttacking = false;

    m_p1.attackFrameIndex = 0;
    m_p2.attackFrameIndex = 0;

    m_p1.attackTickAccum = 0;
    m_p2.attackTickAccum = 0;

    m_p1.hp = MAX_HP;
    m_p2.hp = MAX_HP;

    m_p1.hasHitDuringAttack = false;
    m_p2.hasHitDuringAttack = false;

    m_roundOver = false;
    m_winnerText.clear();

    m_projectiles.clear();
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
    fighter.isMoving = isMoving;

    if (fighter.isAttacking) {
        ++fighter.attackTickAccum;

        if (fighter.attackTickAccum >= ATTACK_TICKS_PER_FRAME) {
            fighter.attackTickAccum = 0;
            ++fighter.attackFrameIndex;

            if (fighter.attackFrameIndex >= ATTACK_FRAMES) {
                fighter.isAttacking = false;
                fighter.attackFrameIndex = 0;
                fighter.attackTickAccum = 0;
                fighter.hasHitDuringAttack = false;
            }
        }

        return;
    }

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

void PvpArenaWidget::startAttack(PvpFighterAnim& fighter, int owner)
{
    if (m_roundOver)
        return;

    if (fighter.attackSheet.isNull())
        return;

    if (fighter.isAttacking)
        return;

    fighter.isAttacking = true;
    fighter.attackFrameIndex = 0;
    fighter.attackTickAccum = 0;
    fighter.hasHitDuringAttack = false;

    if (fighter.type == CharacterType::Archer ||
        fighter.type == CharacterType::Mage) {
        spawnProjectile(fighter, owner);
    }

    update();
}

QPointF PvpArenaWidget::directionVector(PvpDirection dir) const
{
    switch (dir) {
    case PvpDirection::Down:
        return QPointF(0.0, 1.0);
    case PvpDirection::DownRight:
        return QPointF(0.7071, 0.7071);
    case PvpDirection::Right:
        return QPointF(1.0, 0.0);
    case PvpDirection::UpRight:
        return QPointF(0.7071, -0.7071);
    case PvpDirection::Up:
        return QPointF(0.0, -1.0);
    case PvpDirection::UpLeft:
        return QPointF(-0.7071, -0.7071);
    case PvpDirection::Left:
        return QPointF(-1.0, 0.0);
    case PvpDirection::DownLeft:
        return QPointF(-0.7071, 0.7071);
    }

    return QPointF(1.0, 0.0);
}

void PvpArenaWidget::spawnProjectile(const PvpFighterAnim& fighter, int owner)
{
    QPointF dir = directionVector(fighter.facing);

    PvpProjectile projectile;
    projectile.type = fighter.type;
    projectile.owner = owner;
    projectile.lifeTicks = 0;

    projectile.pos = QPointF(
        fighter.pos.x() + PLAYER_W / 2.0,
        fighter.pos.y() + PLAYER_H / 2.0
    ) + dir * 42.0;

    projectile.velocity = dir * PROJECTILE_SPEED;

    m_projectiles.append(projectile);
}

void PvpArenaWidget::updateProjectiles()
{
    for (int i = m_projectiles.size() - 1; i >= 0; --i) {
        PvpProjectile& projectile = m_projectiles[i];

        projectile.pos += projectile.velocity;
        projectile.lifeTicks++;

        const bool outOfBounds =
            projectile.pos.x() < 0.0 ||
            projectile.pos.x() > WORLD_W ||
            projectile.pos.y() < 0.0 ||
            projectile.pos.y() > WORLD_H;

        if (outOfBounds || projectile.lifeTicks > PROJECTILE_MAX_TICKS) {
            m_projectiles.removeAt(i);
        }
    }
}

void PvpArenaWidget::drawProjectiles(QPainter& painter)
{
    painter.save();

    for (const PvpProjectile& projectile : m_projectiles) {
        QPointF dir = projectile.velocity;

        const qreal length = qSqrt(dir.x() * dir.x() + dir.y() * dir.y());
        if (length <= 0.0)
            continue;

        dir /= length;

        if (projectile.type == CharacterType::Archer) {
            QPen arrowPen(QColor("#FDE68A"), 4);
            arrowPen.setCapStyle(Qt::RoundCap);

            painter.setPen(arrowPen);
            painter.setBrush(QColor("#FACC15"));

            QPointF tail = projectile.pos - dir * 24.0;
            QPointF head = projectile.pos;

            painter.drawLine(tail, head);
            painter.drawEllipse(head, 4.0, 4.0);
        } else if (projectile.type == CharacterType::Mage) {
            painter.setPen(QPen(QColor("#FDBA74"), 2));
            painter.setBrush(QColor("#F97316"));
            painter.drawEllipse(projectile.pos, 9.0, 9.0);

            painter.setBrush(QColor("#FACC15"));
            painter.drawEllipse(projectile.pos, 4.0, 4.0);
        }
    }

    painter.restore();
}

QRectF PvpArenaWidget::fighterRect(const PvpFighterAnim& fighter) const
{
    return QRectF(
        fighter.pos.x() + 10.0,
        fighter.pos.y() + 10.0,
        PLAYER_W - 20.0,
        PLAYER_H - 14.0
    );
}

QRectF PvpArenaWidget::meleeHitBox(const PvpFighterAnim& fighter) const
{
    QPointF dir = directionVector(fighter.facing);

    QPointF center(
        fighter.pos.x() + PLAYER_W / 2.0,
        fighter.pos.y() + PLAYER_H / 2.0
    );

    QPointF hitCenter = center + dir * 48.0;

    return QRectF(
        hitCenter.x() - 28.0,
        hitCenter.y() - 28.0,
        56.0,
        56.0
    );
}

void PvpArenaWidget::applyDamage(PvpFighterAnim& target,
                                 int damage,
                                 const QString& winnerText)
{
    if (m_roundOver)
        return;

    target.hp -= damage;

    if (target.hp <= 0) {
        target.hp = 0;
        m_roundOver = true;
        m_winnerText = winnerText;

        // Do NOT clear projectiles here.
        // updateCombatHits() may still be looping through m_projectiles.
    }
}

void PvpArenaWidget::updateCombatHits()
{
    if (m_roundOver)
        return;

    QRectF p1Rect = fighterRect(m_p1);
    QRectF p2Rect = fighterRect(m_p2);

    // Projectile hits
    for (int i = m_projectiles.size() - 1; i >= 0; --i) {
        const PvpProjectile projectile = m_projectiles[i];

        QRectF projectileRect(
            projectile.pos.x() - 8.0,
            projectile.pos.y() - 8.0,
            16.0,
            16.0
        );

        int damage = PROJECTILE_DAMAGE;

        if (projectile.type == CharacterType::Mage)
            damage = MAGE_PROJECTILE_DAMAGE;

        if (projectile.owner == 1 && projectileRect.intersects(p2Rect)) {
            m_projectiles.removeAt(i);
            applyDamage(m_p2, damage, "PLAYER 1 WINS");

            if (m_roundOver) {
                m_projectiles.clear();
                return;
            }

            continue;
        }

        if (projectile.owner == 2 && projectileRect.intersects(p1Rect)) {
            m_projectiles.removeAt(i);
            applyDamage(m_p1, damage, "PLAYER 2 WINS");

            if (m_roundOver) {
                m_projectiles.clear();
                return;
            }

            continue;
        }
    }

    // Warrior melee hits
    if (m_p1.type == CharacterType::Warrior &&
        m_p1.isAttacking &&
        !m_p1.hasHitDuringAttack &&
        m_p1.attackFrameIndex >= 2 &&
        meleeHitBox(m_p1).intersects(p2Rect)) {

        applyDamage(m_p2, MELEE_DAMAGE, "PLAYER 1 WINS");
        m_p1.hasHitDuringAttack = true;

        if (m_roundOver) {
            m_projectiles.clear();
            return;
        }
    }

    if (m_p2.type == CharacterType::Warrior &&
        m_p2.isAttacking &&
        !m_p2.hasHitDuringAttack &&
        m_p2.attackFrameIndex >= 2 &&
        meleeHitBox(m_p2).intersects(p1Rect)) {

        applyDamage(m_p1, MELEE_DAMAGE, "PLAYER 2 WINS");
        m_p2.hasHitDuringAttack = true;

        if (m_roundOver) {
            m_projectiles.clear();
            return;
        }
    }
}

void PvpArenaWidget::onTick()
{
    if (m_roundOver) {
        update();
        return;
    }

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

    updateProjectiles();
    updateCombatHits();

    update();
}

QPixmap PvpArenaWidget::cropFrame(const PvpFighterAnim& fighter) const
{
    const int dirIndex = static_cast<int>(fighter.facing);

    if (fighter.isAttacking && !fighter.attackSheet.isNull()) {
        return fighter.attackSheet.copy(
            fighter.attackFrameIndex * FRAME_W,
            dirIndex * FRAME_H,
            FRAME_W,
            FRAME_H
        );
    }

    if (fighter.movementSheet.isNull())
        return QPixmap();

    if (fighter.isMoving) {
        return fighter.movementSheet.copy(
            fighter.frameIndex * FRAME_W,
            (dirIndex + 1) * FRAME_H,
            FRAME_W,
            FRAME_H
        );
    }

    return fighter.movementSheet.copy(
        dirIndex * FRAME_W,
        0,
        FRAME_W,
        FRAME_H
    );
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

    drawProjectiles(painter);

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
        "P1: WASD + F Attack    |    P2: Arrows + / or K Attack    |    R: Restart"
    );

    if (m_roundOver) {
        QRect overlayRect(0, 0, width(), height());

        painter.fillRect(overlayRect, QColor(0, 0, 0, 150));

        painter.setPen(QColor("#FACC15"));
        painter.setFont(QFont("Arial", 34, QFont::Bold));

        painter.drawText(
            overlayRect,
            Qt::AlignCenter,
            m_winnerText + "\n\nPress R to restart or ESC to leave"
        );
    }
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

    QPixmap frame = cropFrame(fighter);

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

    QRectF hpBack(
        body.x() - 4.0,
        body.y() - 42.0,
        body.width() + 8.0,
        8.0
    );

    qreal hpPercent = static_cast<qreal>(fighter.hp) / MAX_HP;
    hpPercent = qBound(0.0, hpPercent, 1.0);

    QRectF hpFill = hpBack;
    hpFill.setWidth(hpBack.width() * hpPercent);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(30, 30, 30, 210));
    painter.drawRect(hpBack);

    painter.setBrush(QColor("#22C55E"));
    painter.drawRect(hpFill);

    painter.setPen(QPen(QColor("#FFFFFF"), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(hpBack);
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

    if (event->key() == Qt::Key_R && m_roundOver) {
        resetPlayers();
        update();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_F) {
        startAttack(m_p1, 1);
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Slash || event->key() == Qt::Key_K) {
        startAttack(m_p2, 2);
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
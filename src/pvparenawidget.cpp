#include "pvparenawidget.h"

#include <QColor>
#include <QFont>
#include <QKeyEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QPolygonF>
#include <QString>
#include <QtGlobal>
#include <QtMath>

static Direction toSharedDirection(PvpDirection dir)
{
    switch (dir) {
    case PvpDirection::Down:      return Direction::Down;
    case PvpDirection::DownRight: return Direction::DownRight;
    case PvpDirection::Right:     return Direction::Right;
    case PvpDirection::UpRight:   return Direction::ForwardRight;
    case PvpDirection::Up:        return Direction::Up;
    case PvpDirection::UpLeft:    return Direction::ForwardLeft;
    case PvpDirection::Left:      return Direction::Left;
    case PvpDirection::DownLeft:  return Direction::DownLeft;
    }

    return Direction::Down;
}

static PvpDirection fromSharedDirection(Direction dir)
{
    switch (dir) {
    case Direction::Down:         return PvpDirection::Down;
    case Direction::DownRight:    return PvpDirection::DownRight;
    case Direction::Right:        return PvpDirection::Right;
    case Direction::ForwardRight: return PvpDirection::UpRight;
    case Direction::Up:           return PvpDirection::Up;
    case Direction::ForwardLeft:  return PvpDirection::UpLeft;
    case Direction::Left:         return PvpDirection::Left;
    case Direction::DownLeft:     return PvpDirection::DownLeft;
    }

    return PvpDirection::Down;
}

PvpArenaWidget::PvpArenaWidget(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);

    m_arenaBackground.load(":/backgrounds/pvp_arena.png");
    m_arrowProjectileSprite.load(":/sprites/arrow.png");
    m_fireballProjectileSprite.load(":/sprites/fireball.png");

    setFighters(CharacterType::Warrior, CharacterType::Archer);
    resetMatch();

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

void PvpArenaWidget::setMatchTarget(int roundsToWin)
{
    m_roundsToWin = qBound(1, roundsToWin, 3);
    resetMatch();
}

void PvpArenaWidget::resetMatch()
{
    m_p1RoundWins = 0;
    m_p2RoundWins = 0;
    m_matchOver = false;
    resetPlayers();
}

QString PvpArenaWidget::matchModeText() const
{
    if (m_roundsToWin == 1)
        return "Single Round";

    if (m_roundsToWin == 2)
        return "Best of 3";

    return "Best of 5";
}

void PvpArenaWidget::finishRound(const QString& winnerText)
{
    if (m_roundOver)
        return;

    m_roundOver = true;
    m_projectiles.clear();

    if (winnerText.contains("PLAYER 1")) {
        ++m_p1RoundWins;

        if (m_p1RoundWins >= m_roundsToWin) {
            m_matchOver = true;
            m_winnerText = "PLAYER 1 WINS MATCH";
        } else {
            m_winnerText = "PLAYER 1 WINS ROUND";
        }
    } else {
        ++m_p2RoundWins;

        if (m_p2RoundWins >= m_roundsToWin) {
            m_matchOver = true;
            m_winnerText = "PLAYER 2 WINS MATCH";
        } else {
            m_winnerText = "PLAYER 2 WINS ROUND";
        }
    }
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

int PvpArenaWidget::maxHpFor(CharacterType type) const
{
    return m_logic.maxHpFor(type);
}

int PvpArenaWidget::projectileDamageFor(CharacterType type) const
{
    return m_logic.projectileDamageFor(type);
}

int PvpArenaWidget::meleeDamageFor(CharacterType type) const
{
    return m_logic.meleeDamageFor(type);
}

void PvpArenaWidget::setFighters(CharacterType p1Type, CharacterType p2Type)
{
    m_p1.type = p1Type;
    m_p2.type = p2Type;

    m_p1.maxHp = maxHpFor(p1Type);
    m_p2.maxHp = maxHpFor(p2Type);

    m_p1.hp = m_p1.maxHp;
    m_p2.hp = m_p2.maxHp;

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

    m_p1.isBlocking = false;
    m_p2.isBlocking = false;

    m_p1.blockTickAccum = 0;
    m_p2.blockTickAccum = 0;

    m_p1.maxHp = maxHpFor(m_p1.type);
    m_p2.maxHp = maxHpFor(m_p2.type);

    m_p1.hp = m_p1.maxHp;
    m_p2.hp = m_p2.maxHp;

    m_p1.hasHitDuringAttack = false;
    m_p2.hasHitDuringAttack = false;

    m_p1.hitFlashTicks = 0;
    m_p2.hitFlashTicks = 0;

    m_roundOver = false;
    m_winnerText.clear();

    m_projectiles.clear();
}

bool PvpArenaWidget::isPlayer1Blocking() const
{
    return m_heldKeys.contains(Qt::Key_G);
}

bool PvpArenaWidget::isPlayer2Blocking() const
{
    return m_heldKeys.contains(Qt::Key_L);
}

void PvpArenaWidget::updateBlockState()
{
    m_p1.isBlocking = isPlayer1Blocking() && !m_p1.isAttacking && !m_roundOver;
    m_p2.isBlocking = isPlayer2Blocking() && !m_p2.isAttacking && !m_roundOver;

    if (m_p1.isBlocking)
        ++m_p1.blockTickAccum;
    else
        m_p1.blockTickAccum = 0;

    if (m_p2.isBlocking)
        ++m_p2.blockTickAccum;
    else
        m_p2.blockTickAccum = 0;
}

void PvpArenaWidget::updateHitFlash()
{
    if (m_p1.hitFlashTicks > 0)
        --m_p1.hitFlashTicks;

    if (m_p2.hitFlashTicks > 0)
        --m_p2.hitFlashTicks;
}

QPointF PvpArenaWidget::velocityForPlayer1() const
{
    return m_logic.playerVelocity(m_heldKeys, 1, SPEED, isPlayer1Blocking());
}

QPointF PvpArenaWidget::velocityForPlayer2() const
{
    return m_logic.playerVelocity(m_heldKeys, 2, SPEED, isPlayer2Blocking());
}

QPointF PvpArenaWidget::clampToArena(const QPointF& pos) const
{
    QRectF arenaBounds(170.0, 210.0, WORLD_W - 340.0, WORLD_H - 330.0);
    return m_logic.clampToArena(pos, PLAYER_W, PLAYER_H, arenaBounds);
}

PvpDirection PvpArenaWidget::directionFromVelocity(const QPointF& velocity,
                                                   PvpDirection fallback) const
{
    Direction sharedFallback = toSharedDirection(fallback);
    return fromSharedDirection(m_logic.directionFromVelocity(velocity, sharedFallback));
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

    if ((owner == 1 && isPlayer1Blocking()) ||
        (owner == 2 && isPlayer2Blocking()))
        return;

    if (fighter.isBlocking)
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
    return m_logic.directionVector(toSharedDirection(dir));
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

        const qreal angle = qRadiansToDegrees(qAtan2(dir.y(), dir.x()));

        painter.save();
        painter.translate(projectile.pos);
        painter.rotate(angle);

        if (projectile.type == CharacterType::Archer) {
            if (!m_arrowProjectileSprite.isNull()) {
                QRectF target(-24.0, -9.0, 48.0, 18.0);
                painter.drawPixmap(target,
                                   m_arrowProjectileSprite,
                                   m_arrowProjectileSprite.rect());
            } else {
                QPen arrowPen(QColor("#FDE68A"), 4);
                arrowPen.setCapStyle(Qt::RoundCap);

                painter.setPen(arrowPen);
                painter.setBrush(QColor("#FACC15"));

                painter.drawLine(QPointF(-24.0, 0.0), QPointF(24.0, 0.0));
                painter.drawEllipse(QPointF(24.0, 0.0), 4.0, 4.0);
            }
        } else if (projectile.type == CharacterType::Mage) {
            if (!m_fireballProjectileSprite.isNull()) {
                QRectF target(-20.0, -20.0, 40.0, 40.0);
                painter.drawPixmap(target,
                                   m_fireballProjectileSprite,
                                   m_fireballProjectileSprite.rect());
            } else {
                painter.setPen(QPen(QColor("#FDBA74"), 2));
                painter.setBrush(QColor("#F97316"));
                painter.drawEllipse(QPointF(0.0, 0.0), 9.0, 9.0);

                painter.setBrush(QColor("#FACC15"));
                painter.drawEllipse(QPointF(0.0, 0.0), 4.0, 4.0);
            }
        }

        painter.restore();
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

int PvpArenaWidget::finalDamageForTarget(const PvpFighterAnim& target, int damage) const
{
    return m_logic.damageAfterBlock(damage, target.isBlocking);
}

void PvpArenaWidget::applyKnockback(PvpFighterAnim& target, const QPointF& sourcePos)
{
    if (target.isBlocking)
        return;

    QPointF targetCenter(
        target.pos.x() + PLAYER_W / 2.0,
        target.pos.y() + PLAYER_H / 2.0
    );

    QPointF delta = targetCenter - sourcePos;

    qreal length = qSqrt(delta.x() * delta.x() + delta.y() * delta.y());

    if (length <= 0.001) {
        delta = QPointF(1.0, 0.0);
        length = 1.0;
    }

    QPointF dir(delta.x() / length, delta.y() / length);

    target.pos = clampToArena(target.pos + dir * KNOCKBACK_DISTANCE);
}

void PvpArenaWidget::applyDamage(PvpFighterAnim& target,
                                 int damage,
                                 const QString& winnerText)
{
    if (m_roundOver)
        return;

    damage = finalDamageForTarget(target, damage);

    target.hp -= damage;
    target.hitFlashTicks = HIT_FLASH_TICKS;

    if (target.hp <= 0) {
        target.hp = 0;
        finishRound(winnerText);
    }
}

void PvpArenaWidget::updateCombatHits()
{
    if (m_roundOver)
        return;

    QRectF p1Rect = fighterRect(m_p1);
    QRectF p2Rect = fighterRect(m_p2);

    for (int i = m_projectiles.size() - 1; i >= 0; --i) {
        const PvpProjectile projectile = m_projectiles[i];

        QRectF projectileRect(
            projectile.pos.x() - 8.0,
            projectile.pos.y() - 8.0,
            16.0,
            16.0
        );

        const int damage = projectileDamageFor(projectile.type);

        if (projectile.owner == 1 && projectileRect.intersects(p2Rect)) {
            m_projectiles.removeAt(i);

            applyKnockback(m_p2, projectile.pos);
            applyDamage(m_p2, damage, "PLAYER 1 WINS");

            if (m_roundOver)
                return;

            continue;
        }

        if (projectile.owner == 2 && projectileRect.intersects(p1Rect)) {
            m_projectiles.removeAt(i);

            applyKnockback(m_p1, projectile.pos);
            applyDamage(m_p1, damage, "PLAYER 2 WINS");

            if (m_roundOver)
                return;

            continue;
        }
    }

    if (m_p1.type == CharacterType::Warrior &&
        m_p1.isAttacking &&
        !m_p1.hasHitDuringAttack &&
        m_p1.attackFrameIndex >= 2 &&
        meleeHitBox(m_p1).intersects(p2Rect)) {

        QPointF p1Center(
            m_p1.pos.x() + PLAYER_W / 2.0,
            m_p1.pos.y() + PLAYER_H / 2.0
        );

        applyKnockback(m_p2, p1Center);
        applyDamage(m_p2, meleeDamageFor(m_p1.type), "PLAYER 1 WINS");
        m_p1.hasHitDuringAttack = true;

        if (m_roundOver)
            return;
    }

    if (m_p2.type == CharacterType::Warrior &&
        m_p2.isAttacking &&
        !m_p2.hasHitDuringAttack &&
        m_p2.attackFrameIndex >= 2 &&
        meleeHitBox(m_p2).intersects(p1Rect)) {

        QPointF p2Center(
            m_p2.pos.x() + PLAYER_W / 2.0,
            m_p2.pos.y() + PLAYER_H / 2.0
        );

        applyKnockback(m_p1, p2Center);
        applyDamage(m_p1, meleeDamageFor(m_p2.type), "PLAYER 2 WINS");
        m_p2.hasHitDuringAttack = true;

        if (m_roundOver)
            return;
    }
}

void PvpArenaWidget::onTick()
{
    if (m_roundOver) {
        update();
        return;
    }

    updateBlockState();
    updateHitFlash();

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

    painter.setPen(QColor("#F9FAFB"));
    painter.setFont(QFont("Arial", 15, QFont::Bold));
    painter.drawText(
        QRect(0, 10, width(), 32),
        Qt::AlignCenter,
        QString("%1    P1 %2 - %3 P2")
            .arg(matchModeText())
            .arg(m_p1RoundWins)
            .arg(m_p2RoundWins)
    );

    QRect controlsRect(0, height() - 46, width(), 34);

    painter.fillRect(
        controlsRect.adjusted(0, -6, 0, 6),
        QColor(0, 0, 0, 150)
    );

    painter.setPen(QColor("#F9FAFB"));
    painter.setFont(QFont("Arial", 13, QFont::Bold));
    painter.drawText(
        controlsRect,
        Qt::AlignCenter,
        "P1: WASD + F Attack + G Block    |    P2: Arrows + / or K Attack + L Block    |    R: Next / Restart"
    );

    if (m_roundOver) {
        QRect overlayRect(0, 0, width(), height());

        painter.fillRect(overlayRect, QColor(0, 0, 0, 150));

        QString instruction = m_matchOver
            ? "Press R to restart match or ESC to leave"
            : "Press R for next round or ESC to leave";

        painter.setPen(QColor("#FACC15"));
        painter.setFont(QFont("Arial", 34, QFont::Bold));

        painter.drawText(
            overlayRect,
            Qt::AlignCenter,
            m_winnerText + "\n\n" + instruction
        );
    }
}

void PvpArenaWidget::drawBlockEffect(QPainter& painter, const PvpFighterAnim& fighter)
{
    if (!fighter.isBlocking)
        return;

    QRectF body(fighter.pos.x(), fighter.pos.y(), PLAYER_W, PLAYER_H);

    QPointF center(
        body.x() + body.width() / 2.0,
        body.y() + body.height() / 2.0
    );

    QPointF dir = directionVector(fighter.facing);

    const qreal angle = qRadiansToDegrees(qAtan2(dir.y(), dir.x()));
    const qreal pulse = 1.0 + 0.08 * qSin(fighter.blockTickAccum * 0.25);

    if (fighter.type == CharacterType::Mage) {
        painter.save();

        painter.setPen(QPen(QColor(34, 197, 94, 210), 4));
        painter.setBrush(QColor(34, 197, 94, 60));

        QRectF barrier(
            center.x() - 46.0 * pulse,
            center.y() - 54.0 * pulse,
            92.0 * pulse,
            108.0 * pulse
        );

        painter.drawEllipse(barrier);

        painter.setPen(QPen(QColor(187, 247, 208, 190), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(barrier.adjusted(7, 7, -7, -7));

        painter.restore();
        return;
    }

    painter.save();

    QPointF shieldCenter = center + dir * 44.0;

    painter.translate(shieldCenter);
    painter.rotate(angle);

    if (fighter.type == CharacterType::Warrior) {
        QPolygonF shield;
        shield << QPointF(-16.0, -24.0)
               << QPointF(16.0, -24.0)
               << QPointF(22.0, -4.0)
               << QPointF(0.0, 28.0)
               << QPointF(-22.0, -4.0);

        painter.setPen(QPen(QColor("#FACC15"), 3));
        painter.setBrush(QColor(160, 160, 170, 230));
        painter.drawPolygon(shield);

        painter.setPen(QPen(QColor("#E5E7EB"), 2));
        painter.drawLine(QPointF(0.0, -20.0), QPointF(0.0, 20.0));
        painter.drawLine(QPointF(-13.0, -5.0), QPointF(13.0, -5.0));
    } else if (fighter.type == CharacterType::Archer) {
        QRectF woodShield(-21.0, -26.0, 42.0, 52.0);

        painter.setPen(QPen(QColor("#78350F"), 3));
        painter.setBrush(QColor(146, 64, 14, 230));
        painter.drawRoundedRect(woodShield, 8.0, 8.0);

        painter.setPen(QPen(QColor("#FDE68A"), 2));
        painter.drawLine(QPointF(-8.0, -22.0), QPointF(-8.0, 22.0));
        painter.drawLine(QPointF(8.0, -22.0), QPointF(8.0, 22.0));

        painter.setPen(QPen(QColor("#451A03"), 2));
        painter.drawLine(QPointF(-17.0, 0.0), QPointF(17.0, 0.0));
    }

    painter.restore();
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

    if (fighter.hitFlashTicks > 0) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 0, 0, 95));
        painter.drawRoundedRect(body, 8, 8);
    }

    drawBlockEffect(painter, fighter);

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

    if (fighter.isBlocking) {
        painter.setPen(QColor("#BBF7D0"));
        painter.setFont(QFont("Arial", 9, QFont::Bold));
        painter.drawText(
            QRectF(body.x() - 20, body.y() - 58, body.width() + 40, 16),
            Qt::AlignCenter,
            "BLOCK"
        );
    }

    QRectF hpBack(
        body.x() - 4.0,
        body.y() - 42.0,
        body.width() + 8.0,
        8.0
    );

    qreal hpPercent = 0.0;

    if (fighter.maxHp > 0)
        hpPercent = static_cast<qreal>(fighter.hp) / fighter.maxHp;

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
        if (m_matchOver)
            resetMatch();
        else
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

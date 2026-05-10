#include "dungeonwidget.h"

#include <algorithm>
#include <cmath>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QGraphicsEllipseItem>
#include <QRadialGradient>
#include <QRandomGenerator>
#include <QDebug>
#include <QPixmap>
#include <QtGlobal>

#include "audiomanager.h"
#include "spritecache.h"
#include "pauseoverlaywidget.h"
#include "overworldwidget.h"

// ═════════════════════════════════════════════════════════════════════════════
//  Player movement / attack sprite sheet helpers
// ═════════════════════════════════════════════════════════════════════════════

static QString movementSheetFor(CharacterType type)
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

static QString attackSheetFor(CharacterType type)
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

// ═════════════════════════════════════════════════════════════════════════════
//  DungeonPlayerSprite
// ═════════════════════════════════════════════════════════════════════════════

DungeonPlayerSprite::DungeonPlayerSprite(const DungeonSpriteSheet& sheet,
                                         const DungeonAttackSheet& attackSheet,
                                         QGraphicsItem* parent)
    : QGraphicsPixmapItem(parent)
    , m_sheet(sheet)
    , m_attackSheet(attackSheet)
{
    setTransformationMode(Qt::FastTransformation);
    setTransformOriginPoint(W / 2.0, H / 2.0);

    setIdleFrame(Direction::Down);

    const qreal shadowW = W * 0.65;
    const qreal shadowH = H * 0.30;

    m_shadow = new QGraphicsEllipseItem(0, 0, shadowW, shadowH, this);

    QRadialGradient grad(shadowW / 2.0, shadowH / 2.0, shadowW / 2.0);
    grad.setColorAt(0.0, QColor(0, 0, 0, 160));
    grad.setColorAt(0.6, QColor(0, 0, 0, 90));
    grad.setColorAt(1.0, QColor(0, 0, 0, 0));

    m_shadow->setBrush(grad);
    m_shadow->setPen(Qt::NoPen);
    m_shadow->setPos((W - shadowW) / 2.0, H * 0.78);
    m_shadow->setZValue(-1);
}

void DungeonPlayerSprite::setIdleFrame(Direction dir)
{
    m_isIdle = true;
    m_facing = dir;
    m_frameIndex = 0;
    m_tickAccum = 0;
    applyFrame();
}

void DungeonPlayerSprite::setWalkAnim(Direction dir)
{
    if (!m_isIdle && m_facing == dir)
        return;

    m_isIdle = false;
    m_facing = dir;
    m_frameIndex = 0;
    m_tickAccum = 0;
    applyFrame();
}

void DungeonPlayerSprite::applyFrame()
{
    int col;
    int row;

    if (m_isIdle) {
        row = 0;
        col = idleCol(m_facing);
    } else {
        row = walkRow(m_facing);
        col = m_frameIndex;
    }

    QPixmap raw = m_sheet.frame(col, row);

    setPixmap(raw.scaled(
        static_cast<int>(W),
        static_cast<int>(H),
        Qt::KeepAspectRatio,
        Qt::FastTransformation
    ));
}

void DungeonPlayerSprite::applyAttackFrame()
{
    if (m_attackSheet.pixmap.isNull()) {
        applyFrame();
        return;
    }

    const int row = walkRow(m_facing) - 1;
    const int col = m_attackFrameIndex;

    QPixmap raw = m_attackSheet.frame(col, row);

    setPixmap(raw.scaled(
        static_cast<int>(W),
        static_cast<int>(H),
        Qt::KeepAspectRatio,
        Qt::FastTransformation
    ));
}

void DungeonPlayerSprite::updateAnimation(bool isMoving, Direction facingDir)
{
    if (m_isAttacking) {
        ++m_attackTickAccum;

        if (m_attackTickAccum >= ATTACK_TICKS_PER_FRAME) {
            m_attackTickAccum = 0;
            ++m_attackFrameIndex;

            if (m_attackFrameIndex >= ATTACK_FRAMES) {
                m_isAttacking = false;
                m_attackFrameIndex = 0;
                setIdleFrame(m_facing);
                return;
            }

            applyAttackFrame();
        }

        return;
    }

    if (isMoving) {
        setWalkAnim(facingDir);
        ++m_tickAccum;

        if (m_tickAccum >= TICKS_PER_FRAME) {
            m_tickAccum = 0;
            m_frameIndex = (m_frameIndex + 1) % WALK_FRAMES;
            applyFrame();
        }
    } else {
        if (!m_isIdle)
            setIdleFrame(m_facing);
    }
}

void DungeonPlayerSprite::refreshFrame()
{
    applyFrame();
}

void DungeonPlayerSprite::startAttackAnimation(Direction dir)
{
    if (m_attackSheet.pixmap.isNull())
        return;

    m_isAttacking = true;
    m_isIdle = false;
    m_facing = dir;

    m_attackFrameIndex = 0;
    m_attackTickAccum = 0;

    applyAttackFrame();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Enemy monster sprite sheet helpers
// ═════════════════════════════════════════════════════════════════════════════

static QString enemyMovementSheetFor(CharacterType type)
{
    switch (type) {
    case CharacterType::Warrior:
        return ":/sprites/skeleton_guard_movement_8dir_6frames.png";
    case CharacterType::Mage:
        return ":/sprites/goblin_brute_movement_8dir_6frames.png";
    case CharacterType::Archer:
        return ":/sprites/goblin_movement_8dir_6frames.png";
    }

    return ":/sprites/goblin_movement_8dir_6frames.png";
}

static QString enemyAttackSheetFor(CharacterType type)
{
    switch (type) {
    case CharacterType::Warrior:
        return ":/sprites/skeleton_guard_attack_8dir_7frames.png";
    case CharacterType::Mage:
        return ":/sprites/goblin_brute_attack_8dir_7frames.png";
    case CharacterType::Archer:
        return ":/sprites/goblin_attack_8dir_7frames.png";
    }

    return ":/sprites/goblin_attack_8dir_7frames.png";
}

// ═════════════════════════════════════════════════════════════════════════════
//  EnemySprite
// ═════════════════════════════════════════════════════════════════════════════

EnemySprite::EnemySprite(CharacterType type,
                         const QString& name,
                         QGraphicsItem* parent)
    : QGraphicsPixmapItem(parent)
    , m_type(type)
    , m_name(name)
{
    m_movementSheet = SpriteCache::instance().get(enemyMovementSheetFor(type));
    m_attackSheet = SpriteCache::instance().get(enemyAttackSheetFor(type));

    setTransformationMode(Qt::FastTransformation);

    m_facing = Direction::Down;
    m_frameIndex = 0;
    m_tickAccum = 0;
    m_attackFrameIndex = 0;
    m_attackTickAccum = 0;
    m_isMoving = false;
    m_isAttacking = false;

    applyFrame();

    const qreal shadowW = W * 0.70;
    const qreal shadowH = H * 0.25;

    m_shadow = new QGraphicsEllipseItem(0, 0, shadowW, shadowH, this);

    QRadialGradient grad(shadowW / 2.0, shadowH / 2.0, shadowW / 2.0);
    grad.setColorAt(0.0, QColor(0, 0, 0, 140));
    grad.setColorAt(0.6, QColor(0, 0, 0, 70));
    grad.setColorAt(1.0, QColor(0, 0, 0, 0));

    m_shadow->setBrush(grad);
    m_shadow->setPen(Qt::NoPen);
    m_shadow->setPos((W - shadowW) / 2.0, H * 0.75);
    m_shadow->setZValue(-1);

    m_nameLabel = new QGraphicsTextItem(name, this);
    m_nameLabel->setDefaultTextColor(QColor("#ffcccc"));
    m_nameLabel->setFont(QFont("Arial", 6));

    qreal labelW = m_nameLabel->boundingRect().width();
    m_nameLabel->setPos((W - labelW) / 2.0, -14);
    m_nameLabel->setZValue(1);

    m_lastPos = pos();
}

QRectF EnemySprite::hitBox() const
{
    return sceneBoundingRect();
}

void EnemySprite::applyFrame()
{
    if (m_movementSheet.isNull())
        return;

    int row;
    int col;

    if (m_isMoving) {
        row = walkRow(m_facing);
        col = m_frameIndex;
    } else {
        row = 0;
        col = idleCol(m_facing);
    }

    QPixmap raw = m_movementSheet.copy(
        col * FRAME_W,
        row * FRAME_H,
        FRAME_W,
        FRAME_H
    );

    setPixmap(raw.scaled(
        static_cast<int>(W),
        static_cast<int>(H),
        Qt::KeepAspectRatio,
        Qt::FastTransformation
    ));
}

void EnemySprite::applyAttackFrame()
{
    if (m_attackSheet.isNull()) {
        applyFrame();
        return;
    }

    const int row = walkRow(m_facing) - 1;
    const int col = m_attackFrameIndex;

    QPixmap raw = m_attackSheet.copy(
        col * FRAME_W,
        row * FRAME_H,
        FRAME_W,
        FRAME_H
    );

    setPixmap(raw.scaled(
        static_cast<int>(W),
        static_cast<int>(H),
        Qt::KeepAspectRatio,
        Qt::FastTransformation
    ));
}

void EnemySprite::setFacingFromMovement(const QPointF& oldPos,
                                        const QPointF& newPos)
{
    qreal dx = newPos.x() - oldPos.x();
    qreal dy = newPos.y() - oldPos.y();

    if (qAbs(dx) < 0.01 && qAbs(dy) < 0.01)
        return;

    if (qAbs(dx) > qAbs(dy)) {
        if (dx > 0)
            m_facing = Direction::Right;
        else
            m_facing = Direction::Left;
    } else {
        if (dy > 0)
            m_facing = Direction::Down;
        else
            m_facing = Direction::Up;
    }
}

void EnemySprite::chasePlayer(const QRectF& playerBounds,
                              const QRectF& worldBounds)
{
    QPointF oldPos = pos();

    QPointF enemyCenter = sceneBoundingRect().center();
    QPointF playerCenter = playerBounds.center();

    QPointF delta = playerCenter - enemyCenter;

    qreal length = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());

    if (length < 4.0) {
        m_isMoving = false;
        return;
    }

    QPointF dir(delta.x() / length, delta.y() / length);

    static constexpr qreal ENEMY_SPEED = 1.35;

    QPointF nextPos = pos() + dir * ENEMY_SPEED;

    qreal clampedX = std::clamp(
        nextPos.x(),
        worldBounds.left(),
        worldBounds.right() - W
    );

    qreal clampedY = std::clamp(
        nextPos.y(),
        worldBounds.top(),
        worldBounds.bottom() - H
    );

    QPointF newPos(clampedX, clampedY);

    setFacingFromMovement(oldPos, newPos);

    m_isMoving = true;
    setPos(newPos);
}

void EnemySprite::startAttackAnimation(Direction dir)
{
    if (m_attackSheet.isNull())
        return;

    m_isAttacking = true;
    m_isMoving = false;
    m_facing = dir;

    m_attackFrameIndex = 0;
    m_attackTickAccum = 0;

    applyAttackFrame();
}

void EnemySprite::updateAnimation()
{
    if (m_isAttacking) {
        ++m_attackTickAccum;

        if (m_attackTickAccum >= ATTACK_TICKS_PER_FRAME) {
            m_attackTickAccum = 0;
            ++m_attackFrameIndex;

            if (m_attackFrameIndex >= ATTACK_FRAMES) {
                m_isAttacking = false;
                m_attackFrameIndex = 0;
                applyFrame();
                return;
            }

            applyAttackFrame();
        }

        return;
    }

    if (!m_isMoving) {
        m_frameIndex = 0;
        m_tickAccum = 0;
        applyFrame();
        return;
    }

    ++m_tickAccum;

    if (m_tickAccum >= TICKS_PER_FRAME) {
        m_tickAccum = 0;
        m_frameIndex = (m_frameIndex + 1) % WALK_FRAMES;
        applyFrame();
    }

    m_isMoving = false;
}

// ═════════════════════════════════════════════════════════════════════════════
//  DungeonWidget
// ═════════════════════════════════════════════════════════════════════════════

DungeonWidget::DungeonWidget(AudioManager* audio, QWidget* parent)
    : QWidget(parent)
    , m_audio(audio)
{
    loadPlayerSheet(m_playerType);
    loadAttackSheet(m_playerType);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_scene = new QGraphicsScene(0, 0, WORLD_W, WORLD_H, this);
    m_combat.setScene(m_scene);

    connect(&m_combat, &WorldCombatManager::playerDied, this, [this] {
        deactivate();
        updatePlayerHud();

        const int wavesSurvived = std::max(0, m_waveNumber - 1);
        emit dungeonGameOver(m_runCoinsEarned, wavesSurvived);
    });

    m_view = new QGraphicsView(m_scene, this);
    m_view->setFrameShape(QFrame::NoFrame);
    m_view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_view->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    m_view->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(m_view);

    m_ticker.setInterval(16);
    connect(&m_ticker, &QTimer::timeout, this, &DungeonWidget::onTick);

    buildScene();

    setFocusPolicy(Qt::StrongFocus);
    m_view->setSceneRect(0, 0, WORLD_W, WORLD_H);

    m_pauseOverlay = new PauseOverlayWidget(false, this);

    connect(m_pauseOverlay, &PauseOverlayWidget::resumeRequested,
            this, &DungeonWidget::togglePause);

    connect(m_pauseOverlay, &PauseOverlayWidget::menuRequested, this, [this] {
        m_paused = false;
        m_pauseOverlay->hide();
        deactivate();
        emit backToMenu();
    });

    m_goldHud = new GoldHudWidget(this);
    m_goldHud->raise();

    buildPlayerHud();
    updatePlayerHud();
}

void DungeonWidget::activate()
{
    if (m_audio)
        m_audio->playMusic("/music/battle.ogg");

    m_heldKeys.clear();
    m_combat.clearAttacks();
    m_combat.resetSpecialMeter();

    m_waveNumber = 1;
    m_runCoinsEarned = 0;

    placePlayer();
    spawnEnemies();

    updatePlayerHud();
    positionPlayerHud();

    m_ticker.start();
    setFocus();
}

void DungeonWidget::deactivate()
{
    m_ticker.stop();
    m_heldKeys.clear();
    m_combat.clearAttacks();
}

void DungeonWidget::loadPlayerSheet(CharacterType type)
{
    m_sheet.pixmap = SpriteCache::instance().get(movementSheetFor(type));

    if (m_sheet.pixmap.isNull())
        m_sheet.pixmap = SpriteCache::instance().get(":/sprites/player.png");
}

void DungeonWidget::loadAttackSheet(CharacterType type)
{
    m_attackSheet.pixmap = SpriteCache::instance().get(attackSheetFor(type));
}

void DungeonWidget::setPlayerCharacterType(CharacterType type)
{
    m_playerType = type;

    loadPlayerSheet(type);
    loadAttackSheet(type);

    if (m_player)
        m_player->refreshFrame();
}

void DungeonWidget::setPlayerCharacter(Character* player)
{
    m_combat.setPlayer(player);
    updatePlayerHud();
}

void DungeonWidget::buildScene()
{
    m_scene->clear();
    m_scene->setSceneRect(0, 0, WORLD_W, WORLD_H);
    m_combat.setScene(m_scene);

    QPixmap dungeonBg(":/backgrounds/dungeon_bg.png");

    if (!dungeonBg.isNull()) {
        QGraphicsPixmapItem* bgItem = m_scene->addPixmap(
            dungeonBg.scaled(
                WORLD_W,
                WORLD_H,
                Qt::IgnoreAspectRatio,
                Qt::FastTransformation
            )
        );

        bgItem->setPos(0, 0);
        bgItem->setZValue(0);
    } else {
        m_scene->setBackgroundBrush(QColor("#1a1a2e"));
        qWarning("Could not load dungeon background: :/backgrounds/dungeon_bg.png");
    }

    const qreal ew = 80;
    const qreal eh = 56;
    const qreal ex = (WORLD_W - ew) / 2.0;
    const qreal ey = WORLD_H - eh - 6;

    m_exitArea = QRectF(ex, ey, ew, eh);

    // Invisible floor boundary.
    // The player/enemies cannot walk outside this rectangle.
    m_walkableArea = QRectF(
        95,   // left
        155,   // top
        650,  // width
        330   // height
    );

    m_player = new DungeonPlayerSprite(m_sheet, m_attackSheet);
    m_scene->addItem(m_player);
    m_player->setZValue(9);
}

void DungeonWidget::setGold(int gold)
{
    if (m_goldHud)
        m_goldHud->setGold(gold);
}

void DungeonWidget::placePlayer()
{
    if (!m_player)
        return;

    QRectF bounds = m_walkableArea.isValid()
        ? m_walkableArea
        : QRectF(0, 0, WORLD_W, WORLD_H);

    m_player->setPos(
        bounds.center().x() - DungeonPlayerSprite::W / 2.0,
        bounds.bottom() - DungeonPlayerSprite::H - 20.0
    );
}

void DungeonWidget::clearEnemies()
{
    for (EnemySprite* sprite : m_enemies) {
        if (!sprite)
            continue;

        Enemy* enemy = m_enemyLogic.value(sprite, nullptr);

        if (enemy) {
            m_combat.unregisterEnemy(enemy);
            delete enemy;
        }

        m_scene->removeItem(sprite);
        delete sprite;
    }

    m_enemies.clear();
    m_enemyLogic.clear();
    m_combat.clearEnemies();
}

void DungeonWidget::spawnEnemies()
{
    clearEnemies();

    const int enemyCount = std::min(10, 3 + (m_waveNumber - 1) * 2);
    const int hpBonus = (m_waveNumber - 1) * 10;
    const int damageBonus = (m_waveNumber - 1) * 2;

    const QList<QPointF> spawnPoints = {
        QPointF(120, 120),
        QPointF(630, 120),
        QPointF(120, 430),
        QPointF(630, 430),
        QPointF(400, 120),
        QPointF(400, 430),
        QPointF(180, 260),
        QPointF(590, 260)
    };

    for (int i = 0; i < enemyCount; ++i) {
        CharacterType type;

        switch (i % 3) {
        case 0:
            type = CharacterType::Warrior;
            break;
        case 1:
            type = CharacterType::Mage;
            break;
        default:
            type = CharacterType::Archer;
            break;
        }

        QString name;
        int baseHp = 60;
        int baseDamage = 8;

        switch (type) {
        case CharacterType::Warrior:
            name = "Skeleton Guard";
            baseHp = 75;
            baseDamage = 10;
            break;

        case CharacterType::Mage:
            name = "Goblin Brute";
            baseHp = 50;
            baseDamage = 14;
            break;

        case CharacterType::Archer:
            name = "Goblin";
            baseHp = 55;
            baseDamage = 8;
            break;
        }

        QPointF pos = spawnPoints[i % spawnPoints.size()];

        if (i >= spawnPoints.size()) {
            pos += QPointF(
                QRandomGenerator::global()->bounded(-40, 41),
                QRandomGenerator::global()->bounded(-40, 41)
            );
        }

        EnemySprite* sprite = new EnemySprite(type, name);
        sprite->setPos(pos);
        sprite->setZValue(8);

        Enemy* enemy = new Enemy(
            type,
            name,
            baseHp + hpBonus,
            baseDamage + damageBonus
        );

        m_scene->addItem(sprite);
        m_enemies.append(sprite);

        m_enemyLogic.insert(sprite, enemy);
        m_combat.registerEnemy(enemy);
    }
}

void DungeonWidget::startNextWave()
{
    ++m_waveNumber;
    spawnEnemies();
}

void DungeonWidget::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        togglePause();
        return;
    }

    if (e->key() == Qt::Key_J && !e->isAutoRepeat()) {
        handleClassAttack();
        return;
    }

    if (e->key() == Qt::Key_K && !e->isAutoRepeat()) {
        handleSpecialAbility();
        return;
    }

    if (!e->isAutoRepeat())
        m_heldKeys.insert(e->key());

    QWidget::keyPressEvent(e);
}

void DungeonWidget::keyReleaseEvent(QKeyEvent* e)
{
    if (!e->isAutoRepeat())
        m_heldKeys.remove(e->key());

    QWidget::keyReleaseEvent(e);
}

void DungeonWidget::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    fitView();

    if (m_pauseOverlay)
        m_pauseOverlay->setGeometry(rect());

    if (m_goldHud)
        m_goldHud->move(width() - m_goldHud->width() - 16, 16);

    positionPlayerHud();
}

void DungeonWidget::onTick()
{
    if (m_paused)
        return;

    movePlayer();
    patrolEnemies();

    m_combat.update(0.016f);
    checkAttackCollisions();

    checkCollisions();

    if (m_player && m_view)
        m_view->centerOn(m_player);

    updatePlayerHud();
}

    void DungeonWidget::movePlayer()
{
    if (!m_player)
        return;

    m_controller.setSpeed(SPEED);

    QPointF velocity = m_controller.computeVelocity(m_heldKeys);
    QPointF nextPos = m_player->pos() + velocity;

    QRectF bounds = m_walkableArea.isValid()
        ? m_walkableArea
        : QRectF(0, 0, WORLD_W, WORLD_H);

    nextPos.setX(std::clamp(
        nextPos.x(),
        bounds.left(),
        bounds.right() - DungeonPlayerSprite::W
    ));

    nextPos.setY(std::clamp(
        nextPos.y(),
        bounds.top(),
        bounds.bottom() - DungeonPlayerSprite::H
    ));

    m_player->setPos(nextPos);

    bool moving = m_controller.isMoving(m_heldKeys);
    Direction dir = m_controller.computeDirection(m_heldKeys);

    m_player->updateAnimation(moving, dir);

    if (moving)
        m_facing = dir;
}

void DungeonWidget::patrolEnemies()
{
    if (!m_player)
        return;

    QRectF worldBounds = m_walkableArea.isValid()
        ? m_walkableArea
        : QRectF(0, 0, WORLD_W, WORLD_H);

    QRectF playerBounds = m_player->sceneBoundingRect();

    for (EnemySprite* enemy : m_enemies) {
        if (!enemy)
            continue;

        enemy->chasePlayer(playerBounds, worldBounds);
    }

    for (int i = 0; i < m_enemies.size(); ++i) {
        EnemySprite* a = m_enemies[i];

        if (!a)
            continue;

        for (int j = i + 1; j < m_enemies.size(); ++j) {
            EnemySprite* b = m_enemies[j];

            if (!b)
                continue;

            QRectF aBox = a->hitBox();
            QRectF bBox = b->hitBox();

            if (!aBox.intersects(bBox))
                continue;

            QPointF aCenter = aBox.center();
            QPointF bCenter = bBox.center();

            QPointF delta = aCenter - bCenter;

            qreal length = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());

            if (length < 0.001) {
                delta = QPointF(1.0, 0.0);
                length = 1.0;
            }

            QPointF pushDir(delta.x() / length, delta.y() / length);

            QRectF overlap = aBox.intersected(bBox);
            qreal pushAmount = std::min(overlap.width(), overlap.height()) / 2.0 + 0.5;

            QPointF push = pushDir * pushAmount;

            QPointF aNext = a->pos() + push;
            QPointF bNext = b->pos() - push;

            aNext.setX(std::clamp(
                aNext.x(),
                worldBounds.left(),
                worldBounds.right() - EnemySprite::W
            ));

            aNext.setY(std::clamp(
                aNext.y(),
                worldBounds.top(),
                worldBounds.bottom() - EnemySprite::H
            ));

            bNext.setX(std::clamp(
                bNext.x(),
                worldBounds.left(),
                worldBounds.right() - EnemySprite::W
            ));

            bNext.setY(std::clamp(
                bNext.y(),
                worldBounds.top(),
                worldBounds.bottom() - EnemySprite::H
            ));

            a->setPos(aNext);
            b->setPos(bNext);
        }
    }

    for (EnemySprite* enemy : m_enemies) {
        if (!enemy)
            continue;

        enemy->updateAnimation();
    }
}

void DungeonWidget::checkCollisions()
{
    if (!m_player)
        return;

    if (m_exitArea.isValid() &&
        m_player->sceneBoundingRect().intersects(m_exitArea)) {
        deactivate();
        emit exitedDungeon();
        return;
    }

    for (EnemySprite* sprite : m_enemies) {
        if (!sprite)
            continue;

        Enemy* enemy = m_enemyLogic.value(sprite, nullptr);

        if (!enemy || !enemy->isAlive())
            continue;

        QRectF playerHurtBox = m_player->sceneBoundingRect().adjusted(
            28, 28,
            -28, -12
        );

        QRectF enemyMeleeBox = sprite->hitBox().adjusted(
            14, 14,
            -14, -8
        );

        if (playerHurtBox.intersects(enemyMeleeBox) &&
            m_combat.canEnemyAttack(enemy)) {

            int damage = m_combat.enemyAttackDamage(enemy);
            m_combat.damagePlayer(damage);
            m_combat.startEnemyAttackCooldown(enemy);
            sprite->startAttackAnimation(sprite->facingDirection());

            if (m_audio)
                m_audio->playSfx("/sfx/hit.wav");

            updatePlayerHud();
        }
    }
}

void DungeonWidget::fitView()
{
    if (!m_view || !m_player)
        return;

    m_view->resetTransform();
    m_view->scale(2.0, 2.0);
    m_view->centerOn(m_player);
}

void DungeonWidget::togglePause()
{
    m_paused = !m_paused;

    if (!m_pauseOverlay)
        return;

    m_pauseOverlay->setGeometry(rect());

    if (m_paused) {
        m_pauseOverlay->show();
        m_pauseOverlay->raise();
    } else {
        m_pauseOverlay->hide();
        setFocus();
    }
}

void DungeonWidget::handleClassAttack()
{
    if (!m_player)
        return;

    QRectF playerBounds = m_player->sceneBoundingRect();

    ActiveAttack* attack = m_combat.createPlayerAttack(playerBounds, m_facing);

    if (attack) {
        m_player->startAttackAnimation(m_facing);

        if (m_audio)
            m_audio->playSfx("/sfx/special.wav");
    }
}

void DungeonWidget::handleSpecialAbility()
{
    if (!m_player)
        return;

    QRectF playerBounds = m_player->sceneBoundingRect();

    const bool warriorWasActive = m_combat.warriorSpecialActive();

    QList<ActiveAttack*> attacks = m_combat.createPlayerSpecial(playerBounds, m_facing);

    const bool warriorChanged = warriorWasActive != m_combat.warriorSpecialActive();

    if (!attacks.isEmpty() || warriorChanged) {
        m_player->startAttackAnimation(m_facing);

        if (m_audio)
            m_audio->playSfx("/sfx/special.wav");
    }

    updatePlayerHud();
}

void DungeonWidget::checkAttackCollisions()
{
    const QList<ActiveAttack*>& attacks = m_combat.activeAttacks();

    for (ActiveAttack* attack : attacks) {
        if (!attack || attack->expired)
            continue;

        for (EnemySprite* sprite : m_enemies) {
            if (!sprite)
                continue;

            Enemy* enemy = m_enemyLogic.value(sprite, nullptr);

            if (!enemy || !enemy->isAlive())
                continue;

            if (!attack->bounds.intersects(sprite->hitBox()))
                continue;

            if (attack->enemiesHit.contains(enemy))
                continue;

            attack->enemiesHit.insert(enemy);

            m_combat.damageEnemy(enemy, attack->damage);

            if (!attack->piercing)
                m_combat.expireAttack(attack);

            if (m_audio)
                m_audio->playSfx("/sfx/hit.wav");

            if (!attack->piercing)
                break;
        }
    }

    for (int i = m_enemies.size() - 1; i >= 0; --i) {
        EnemySprite* sprite = m_enemies[i];

        if (!sprite)
            continue;

        Enemy* enemy = m_enemyLogic.value(sprite, nullptr);

        if (enemy && !enemy->isAlive()) {
            m_combat.addSpecialFromKill();

            Character* player = m_combat.player();
            if (player && player->isAlive()) {
                int healAmount = std::max(
                    1,
                    static_cast<int>(player->getMaxHealth() * 0.10f)
                );
                player->heal(healAmount);
            }

            m_runCoinsEarned += 3;
            emit goldEarned(3);

            if (m_audio)
                m_audio->playSfx("/sfx/faint.wav");

            m_combat.unregisterEnemy(enemy);
            m_enemyLogic.remove(sprite);
            m_enemies.removeAt(i);

            m_scene->removeItem(sprite);

            delete sprite;
            delete enemy;

            updatePlayerHud();
        }
    }

    if (m_enemies.isEmpty())
        startNextWave();
}

void DungeonWidget::buildPlayerHud()
{
    if (m_playerHud)
        return;

    m_playerHud = new QWidget(this);
    m_playerHud->setFixedSize(250, 78);
    m_playerHud->setAttribute(Qt::WA_StyledBackground, true);
    m_playerHud->setStyleSheet(
        "QWidget {"
        " background-color: rgba(13, 13, 26, 190);"
        " border: 3px solid #F0E8D0;"
        "}"
        "QLabel {"
        " background: transparent;"
        " border: none;"
        " color: #F0E8D0;"
        " font-size: 8px;"
        "}"
    );

    auto* root = new QVBoxLayout(m_playerHud);
    root->setContentsMargins(10, 8, 10, 8);
    root->setSpacing(6);

    auto* healthRow = new QHBoxLayout;
    healthRow->setSpacing(8);

    m_healthLabel = new QLabel("HP", m_playerHud);
    m_healthLabel->setFixedWidth(60);

    m_healthBar = new HealthBarWidget(m_playerHud);
    m_healthBar->setFixedSize(160, 18);

    healthRow->addWidget(m_healthLabel);
    healthRow->addWidget(m_healthBar);

    auto* specialRow = new QHBoxLayout;
    specialRow->setSpacing(8);

    m_specialLabel = new QLabel("SPECIAL", m_playerHud);
    m_specialLabel->setFixedWidth(60);

    m_specialBar = new HealthBarWidget(m_playerHud);
    m_specialBar->setFixedSize(160, 18);
    m_specialBar->setFixedBarColor(QColor("#4A90D9"));

    specialRow->addWidget(m_specialLabel);
    specialRow->addWidget(m_specialBar);

    root->addLayout(healthRow);
    root->addLayout(specialRow);

    positionPlayerHud();
    m_playerHud->raise();
    m_playerHud->show();
}

void DungeonWidget::updatePlayerHud()
{
    if (!m_healthBar || !m_specialBar)
        return;

    Character* player = m_combat.player();

    if (!player) {
        m_healthBar->setBarPercent(0.0f);
        m_specialBar->setBarPercent(0.0f);
        return;
    }

    m_healthBar->setBarPercent(player->getHealthPercent());
    m_specialBar->setBarPercent(m_combat.specialMeter());
}

void DungeonWidget::positionPlayerHud()
{
    if (!m_playerHud)
        return;

    constexpr int margin = 16;

    m_playerHud->move(
        width() - m_playerHud->width() - margin,
        height() - m_playerHud->height() - margin
    );

    m_playerHud->raise();
}
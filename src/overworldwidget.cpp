#include "overworldwidget.h"
#include <QGraphicsEllipseItem>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QKeyEvent>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QRandomGenerator>
#include <QRadialGradient>
#include "audiomanager.h"
#include <QLabel>
#include <QPushButton>

// ═════════════════════════════════════════════════════════════════════════════
//  PlayerSprite
// ═════════════════════════════════════════════════════════════════════════════

PlayerSprite::PlayerSprite(const SpriteSheet &sheet, QGraphicsItem *parent)
    : QGraphicsPixmapItem(parent)
    , m_sheet(sheet)
{
    setTransformationMode(Qt::FastTransformation);
    setTransformOriginPoint(W / 2.0, H / 2.0);

    // Start idle, facing down (toward camera)
    setIdleFrame(Direction::Down);

    // ── Shadow ─────────────────────────────────────────────
    const qreal shadowW = W * 0.65;
    const qreal shadowH = H * 0.30;

    m_shadow = new QGraphicsEllipseItem(0, 0, shadowW, shadowH, this);

    QRadialGradient grad(shadowW / 2.0, shadowH / 2.0, shadowW / 2.0);
    grad.setColorAt(0.0, QColor(0,0,0,140));
    grad.setColorAt(0.6, QColor(0,0,0,80));
    grad.setColorAt(1.0, QColor(0,0,0,0));

    m_shadow->setBrush(grad);
    m_shadow->setPen(Qt::NoPen);
    m_shadow->setPos((W - shadowW) / 2.0, H * 0.78);
    m_shadow->setZValue(-1);
}

void PlayerSprite::setIdleFrame(Direction dir)
{
    m_isIdle     = true;
    m_facing     = dir;
    m_frameIndex = 0;
    m_tickAccum  = 0;
    applyFrame();
}

void PlayerSprite::setWalkAnim(Direction dir)
{
    if (!m_isIdle && m_facing == dir) return;

    m_isIdle     = false;
    m_facing     = dir;
    m_frameIndex = 0;
    m_tickAccum  = 0;
    applyFrame();
}

void PlayerSprite::applyFrame()
{
    int col, row;

    if (m_isIdle) {
        row = 0;
        col = idleCol(m_facing);
    } else {
        row = walkRow(m_facing);
        col = m_frameIndex;
    }

    QPixmap raw = m_sheet.frame(col, row);
    setPixmap(raw.scaled(static_cast<int>(W),
                         static_cast<int>(H),
                         Qt::KeepAspectRatio,
                         Qt::FastTransformation));
}

// ─────────────────────────────────────────────────────────────────────────────

void PlayerSprite::step(const QSet<int> &heldKeys, const QRectF &worldBounds)
{
    const bool goUp    = heldKeys.contains(Qt::Key_W) || heldKeys.contains(Qt::Key_Up);
    const bool goDown  = heldKeys.contains(Qt::Key_S) || heldKeys.contains(Qt::Key_Down);
    const bool goLeft  = heldKeys.contains(Qt::Key_A) || heldKeys.contains(Qt::Key_Left);
    const bool goRight = heldKeys.contains(Qt::Key_D) || heldKeys.contains(Qt::Key_Right);

    qreal dx = 0, dy = 0;
    if (goUp)    dy -= SPEED;
    if (goDown)  dy += SPEED;
    if (goLeft)  dx -= SPEED;
    if (goRight) dx += SPEED;

    if (dx != 0 && dy != 0) { dx *= 0.7071; dy *= 0.7071; }

    const bool moving = (dx != 0 || dy != 0);

    if (moving) {
        Direction dir;
        if      (goUp   && goRight) dir = Direction::ForwardRight;
        else if (goUp   && goLeft)  dir = Direction::ForwardLeft;
        else if (goDown && goRight) dir = Direction::DownRight;
        else if (goDown && goLeft)  dir = Direction::DownLeft;
        else if (goRight)           dir = Direction::Right;
        else if (goLeft)            dir = Direction::Left;
        else if (goUp)              dir = Direction::Up;
        else                        dir = Direction::Down;

        setWalkAnim(dir);

        ++m_tickAccum;
        if (m_tickAccum >= TICKS_PER_FRAME) {
            m_tickAccum  = 0;
            m_frameIndex = (m_frameIndex + 1) % WALK_FRAMES;
            applyFrame();
        }
    } else {
        if (!m_isIdle) {
            setIdleFrame(m_facing);
        }
    }

    qreal nx = qBound(worldBounds.left(),  x() + dx, worldBounds.right()  - W);
    qreal ny = qBound(worldBounds.top(),   y() + dy, worldBounds.bottom() - H);
    setPos(nx, ny);
}


// ═════════════════════════════════════════════════════════════════════════════
//  OverworldWidget
// ═════════════════════════════════════════════════════════════════════════════

OverworldWidget::OverworldWidget(AudioManager *audio, QWidget *parent)
    : QWidget(parent)
    , m_audio(audio)
{
    // ── Load sprite sheet ────────────────────────────────────────────────────
    QString spritePath = ":/sprites/player.png";
    qDebug() << "Working directory:" << QDir::currentPath();
    qDebug() << "Absolute sprite path:" << QFileInfo(spritePath).absoluteFilePath();
    qDebug() << "File exists on disk:" << QFileInfo(spritePath).exists();

    m_sheet.pixmap = QPixmap(spritePath);

    if (m_sheet.pixmap.isNull()) {
        qFatal("Could not load resources/sprites/player.png");
    }

    // ── Layout ───────────────────────────────────────────────────────────────
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_scene = new QGraphicsScene(0, 0, WORLD_W, WORLD_H, this);

    m_view = new QGraphicsView(m_scene, this);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(m_view);

    m_ticker.setInterval(16);
    connect(&m_ticker, &QTimer::timeout, this, &OverworldWidget::onTick);

    buildScene();
    setFocusPolicy(Qt::StrongFocus);
    m_view->setTransform(QTransform());
    m_view->setSceneRect(0, 0, WORLD_W, WORLD_H);

    buildPauseOverlay();
}

// ─────────────────────────────────────────────────────────────────────────────

void OverworldWidget::activate()
{
    if (m_audio) m_audio->playMusic("/music/overworld.ogg");
    m_heldKeys.clear();
    placePlayer();
    m_ticker.start();
    setFocus();
}

void OverworldWidget::deactivate()
{
    m_ticker.stop();
    m_heldKeys.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Scene construction
// ─────────────────────────────────────────────────────────────────────────────

void OverworldWidget::buildScene()
{
    // ── Background grass tiles ───────────────────────────────────────────────
    QPixmap grassTile(":/sprites/grass.png");

    const int TILE_SIZE = 64;

    QPixmap scaledTile = grassTile.scaled(
        TILE_SIZE, TILE_SIZE,
        Qt::IgnoreAspectRatio,
        Qt::FastTransformation);

    for (int y = 0; y < WORLD_H; y += TILE_SIZE) {
        for (int x = 0; x < WORLD_W; x += TILE_SIZE) {
            QGraphicsPixmapItem *tile = m_scene->addPixmap(scaledTile);
            tile->setPos(x, y);
            tile->setZValue(0);
        }
    }

    // ── Dirt path ────────────────────────────────────────────────────────────
    QPixmap dirt1(":/sprites/dirt1.png");
    QPixmap dirt2(":/sprites/dirt2.png");

    const int TILE = 64;

    QPixmap d1 = dirt1.scaled(TILE, TILE, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    QPixmap d2 = dirt2.scaled(TILE, TILE, Qt::IgnoreAspectRatio, Qt::FastTransformation);

    qreal pathX = WORLD_W * 0.38;
    qreal pathW = WORLD_W * 0.24;

    for (int y = 0; y < WORLD_H; y += TILE) {
        for (int x = pathX; x < pathX + pathW; x += TILE) {
            bool useAlt = (QRandomGenerator::global()->bounded(100) < 12);
            QGraphicsPixmapItem *tile = m_scene->addPixmap(useAlt ? d2 : d1);
            tile->setPos(x, y);
            tile->setZValue(1);
        }
    }

    // ── House ────────────────────────────────────────────────────────────────
    static constexpr int HOUSE_W = 240;
    static constexpr int HOUSE_H = 240;
    static constexpr int HOUSE_X = 60;
    static constexpr int HOUSE_Y = 180;

    // Shadow — sheared rectangle simulating sunlight from the top-left
    // The shadow is a parallelogram: same width as the house, cast to the right
    {
        // Shadow rect before shearing: full house width, about 30% of house height
        const qreal shadowW = HOUSE_W * 0.7;
        const qreal shadowH = HOUSE_H * 0.30;

        // Place it so its top edge aligns with the bottom of the house
        auto *houseShadow = m_scene->addRect(
            0, 0, shadowW, shadowH,
            Qt::NoPen, QBrush(QColor(0, 0, 0, 40)));


        // Position: sits at ~60% down the house, not the very bottom
        houseShadow->setPos(HOUSE_X+40, HOUSE_Y+155);
        houseShadow->setZValue(2);   // above ground tiles, below house sprite
    }

    // House sprite
    QPixmap housePx("resources/sprites/house.png");
    if (!housePx.isNull()) {
        QGraphicsPixmapItem *houseItem = m_scene->addPixmap(
            housePx.scaled(HOUSE_W, HOUSE_H,
                           Qt::KeepAspectRatio,
                           Qt::FastTransformation));
        houseItem->setPos(HOUSE_X, HOUSE_Y);
        houseItem->setZValue(10);
    } else {
        qWarning("Could not load resources/sprites/house.png");
    }

    // Invisible collision box — starts at 60%, inset 10% on each side
    // Player hits the wall face, not the air in front of the building
    const qreal colliderInset = HOUSE_W * 0.10;
    const qreal colliderY     = HOUSE_Y + HOUSE_H * 0.60;
    const qreal colliderH     = HOUSE_H * 0.25;
    m_houseCollider = m_scene->addRect(
        HOUSE_X + colliderInset, colliderY,
        HOUSE_W - colliderInset * 2, colliderH,
        Qt::NoPen, Qt::NoBrush);
    m_houseCollider->setZValue(1);

    // ── Trees ────────────────────────────────────────────────────────────────
    const QList<QPointF> trees = {
        {60,  60},  {150, 50},  {680, 55},  {740, 130},
        {60,  430}, {130, 510}, {690, 420}, {750, 300},
        {550, 480}, {580, 80},
    };
    for (const QPointF &p : trees) {
        m_scene->addRect(p.x() + 8, p.y() + 26, 12, 16,
                         Qt::NoPen, QBrush(QColor("#5d4037")))->setZValue(1);
        m_scene->addEllipse(p.x(), p.y(), 28, 32,
                            Qt::NoPen, QBrush(QColor("#2e7d32")))->setZValue(2);
    }

    // ── Dungeon entrance ─────────────────────────────────────────────────────
    const qreal dw = 80, dh = 56;
    const qreal dx = (WORLD_W - dw) / 2.0;
    const qreal dy = 6;

    m_scene->addRect(dx - 10, dy - 6, dw + 20, dh + 10,
                     Qt::NoPen, QBrush(QColor("#424242")))->setZValue(1);

    m_dungeonZone = m_scene->addRect(dx, dy, dw, dh,
                                     Qt::NoPen, QBrush(QColor("#1a1a2e")));
    m_dungeonZone->setZValue(2);

    auto *skull = m_scene->addText("☠  DUNGEON", QFont("Arial", 8, QFont::Bold));
    skull->setDefaultTextColor(QColor("#ef5350"));
    skull->setPos(dx + 2, dy + dh + 4);
    skull->setZValue(3);

    // ── HUD hint ─────────────────────────────────────────────────────────────
    auto *hint = m_scene->addText(
        "WASD / Arrow keys to move    ESC = menu",
        QFont("Arial", 7));
    hint->setDefaultTextColor(QColor("#ffffffaa"));
    hint->setPos(8, WORLD_H - 20);
    hint->setZValue(10);

    // ── Player ───────────────────────────────────────────────────────────────
    m_player = new PlayerSprite(m_sheet);
    m_player->setZValue(9);
    m_scene->addItem(m_player);
    placePlayer();
}

void OverworldWidget::placePlayer()
{
    if (m_player)
        m_player->setPos((WORLD_W - PlayerSprite::W) / 2.0,
                         (WORLD_H - PlayerSprite::H) / 2.0);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Game loop
// ─────────────────────────────────────────────────────────────────────────────

void OverworldWidget::onTick()
{
    m_player->step(m_heldKeys, QRectF(0, 0, WORLD_W, WORLD_H));
    checkTriggers();
}

void OverworldWidget::checkTriggers()
{
    // ── Dungeon entrance ─────────────────────────────────────────────────────
    if (m_player->collidesWithItem(m_dungeonZone)) {
        deactivate();
        emit dungeonEntered();
        return;
    }

    // ── House collision — push player out of the collider box ────────────────
    if (m_houseCollider && m_player->collidesWithItem(m_houseCollider)) {
        const QRectF playerRect   = m_player->mapToScene(
                                        m_player->boundingRect()).boundingRect();
        const QRectF colliderRect = m_houseCollider->sceneBoundingRect();

        const qreal overlapLeft  = playerRect.right()  - colliderRect.left();
        const qreal overlapRight = colliderRect.right() - playerRect.left();
        const qreal overlapTop   = playerRect.bottom()  - colliderRect.top();
        const qreal overlapBot   = colliderRect.bottom() - playerRect.top();

        const qreal minX = qMin(overlapLeft,  overlapRight);
        const qreal minY = qMin(overlapTop,   overlapBot);

        qreal px = m_player->x();
        qreal py = m_player->y();

        if (minX < minY) {
            px += (overlapLeft < overlapRight) ? -overlapLeft : overlapRight;
        } else {
            py += (overlapTop  < overlapBot)   ? -overlapTop  : overlapBot;
        }

        m_player->setPos(px, py);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Input
// ─────────────────────────────────────────────────────────────────────────────

void OverworldWidget::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape) {
        togglePause();
        return;
    }
    if (!m_paused)
        m_heldKeys.insert(e->key());
}

void OverworldWidget::keyReleaseEvent(QKeyEvent *e)
{
    m_heldKeys.remove(e->key());
}

// ─────────────────────────────────────────────────────────────────────────────
//  Resize
// ─────────────────────────────────────────────────────────────────────────────

void OverworldWidget::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    if (m_pauseOverlay) m_pauseOverlay->resize(size());
    fitView();
}

void OverworldWidget::fitView()
{
}

void OverworldWidget::togglePause()
{
    m_paused = !m_paused;
    if (m_paused) {
        m_ticker.stop();
        m_heldKeys.clear();
        m_pauseOverlay->resize(size());
        m_pauseOverlay->show();
        m_pauseOverlay->raise();
    } else {
        m_pauseOverlay->hide();
        m_ticker.start();
        setFocus();
    }
}

void OverworldWidget::buildPauseOverlay()
{
    m_pauseOverlay = new QWidget(this);
    m_pauseOverlay->setAttribute(Qt::WA_TranslucentBackground);
    m_pauseOverlay->hide();

    auto *layout = new QVBoxLayout(m_pauseOverlay);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(16);

    auto *title  = new QLabel("— PAUSED —", m_pauseOverlay);
    auto *resume = new QPushButton("► RESUME",    m_pauseOverlay);
    auto *menu   = new QPushButton("  MAIN MENU", m_pauseOverlay);

    title->setObjectName("titleLabel");
    title->setAlignment(Qt::AlignCenter);

    connect(resume, &QPushButton::clicked, this, &OverworldWidget::togglePause);
    connect(menu,   &QPushButton::clicked, this, [this]{
        m_paused = false;
        m_pauseOverlay->hide();
        deactivate();
        emit backToMenu();
    });

    layout->addWidget(title);
    layout->addSpacing(12);
    layout->addWidget(resume);
    layout->addWidget(menu);
}

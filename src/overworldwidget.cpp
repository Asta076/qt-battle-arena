#include "overworldwidget.h"

#include <QGraphicsEllipseItem>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QKeyEvent>
#include <QShowEvent>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QRandomGenerator>
#include <QRadialGradient>
#include <QLabel>
#include <QPushButton>

#include "audiomanager.h"


// ============================================================
//  PlayerSprite  — constructor
// ============================================================

PlayerSprite::PlayerSprite(const SpriteSheet &sheet, QGraphicsItem *parent)
    : QGraphicsPixmapItem(parent)
    , m_sheet(sheet)
{
    setTransformationMode(Qt::FastTransformation);
    setTransformOriginPoint(W / 2.0, H / 2.0);

    // start facing down (toward the camera)
    setIdleFrame(Direction::Down);

    // build the shadow ellipse that sits under the player's feet
    const qreal shadowW = W * 0.65;
    const qreal shadowH = H * 0.30;

    m_shadow = new QGraphicsEllipseItem(0, 0, shadowW, shadowH, this);

    QRadialGradient grad(shadowW / 2.0, shadowH / 2.0, shadowW / 2.0);
    grad.setColorAt(0.0, QColor(0, 0, 0, 140));
    grad.setColorAt(0.6, QColor(0, 0, 0, 80));
    grad.setColorAt(1.0, QColor(0, 0, 0, 0));

    m_shadow->setBrush(grad);
    m_shadow->setPen(Qt::NoPen);
    m_shadow->setPos((W - shadowW) / 2.0, H * 0.78);
    m_shadow->setZValue(-1);
}


// ============================================================
//  PlayerSprite  — animation helpers
// ============================================================

// switch to idle pose for the given direction
void PlayerSprite::setIdleFrame(Direction dir)
{
    m_isIdle     = true;
    m_facing     = dir;
    m_frameIndex = 0;
    m_tickAccum  = 0;
    applyFrame();
}

// start (or continue) a walk animation for the given direction
void PlayerSprite::setWalkAnim(Direction dir)
{
    // dont restart the animation if we're already walking that way
    if (!m_isIdle && m_facing == dir) return;

    m_isIdle     = false;
    m_facing     = dir;
    m_frameIndex = 0;
    m_tickAccum  = 0;
    applyFrame();
}

// actually set the pixmap to the correct frame based on current state
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
    setPixmap(raw.scaled(
        static_cast<int>(W),
        static_cast<int>(H),
        Qt::KeepAspectRatio,
        Qt::FastTransformation
    ));
}


// ============================================================
//  PlayerSprite  — movement (called every tick)
// ============================================================

void PlayerSprite::step(const QSet<int> &heldKeys, const QRectF &worldBounds)
{
    // figure out which directions are pressed
    const bool goUp    = heldKeys.contains(Qt::Key_W) || heldKeys.contains(Qt::Key_Up);
    const bool goDown  = heldKeys.contains(Qt::Key_S) || heldKeys.contains(Qt::Key_Down);
    const bool goLeft  = heldKeys.contains(Qt::Key_A) || heldKeys.contains(Qt::Key_Left);
    const bool goRight = heldKeys.contains(Qt::Key_D) || heldKeys.contains(Qt::Key_Right);

    // calculate raw movement delta
    qreal dx = 0, dy = 0;
    if (goUp)    dy -= SPEED;
    if (goDown)  dy += SPEED;
    if (goLeft)  dx -= SPEED;
    if (goRight) dx += SPEED;

    // normalize diagonal movement so you dont go faster diagonally
    if (dx != 0 && dy != 0) {
        dx *= 0.7071;
        dy *= 0.7071;
    }

    const bool moving = (dx != 0 || dy != 0);

    if (moving) {
        // pick direction based on which keys are held
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

        // advance animation frame every TICKS_PER_FRAME ticks
        ++m_tickAccum;
        if (m_tickAccum >= TICKS_PER_FRAME) {
            m_tickAccum  = 0;
            m_frameIndex = (m_frameIndex + 1) % WALK_FRAMES;
            applyFrame();
        }
    } else {
        // no keys held — switch to idle if we werent already
        if (!m_isIdle) {
            setIdleFrame(m_facing);
        }
    }

    // clamp position so player cant walk off the edge of the world
    qreal nx = qBound(worldBounds.left(),  x() + dx, worldBounds.right()  - W);
    qreal ny = qBound(worldBounds.top(),   y() + dy, worldBounds.bottom() - H);
    setPos(nx, ny);
}


// ============================================================
//  OverworldWidget  — constructor
// ============================================================

OverworldWidget::OverworldWidget(AudioManager *audio, QWidget *parent)
    : QWidget(parent)
    , m_audio(audio)
{
    // load the player sprite sheet from resources
    QString spritePath = ":/sprites/player.png";
    qDebug() << "Working directory:" << QDir::currentPath();
    qDebug() << "Absolute sprite path:" << QFileInfo(spritePath).absoluteFilePath();
    qDebug() << "File exists on disk:" << QFileInfo(spritePath).exists();

    m_sheet.pixmap = QPixmap(spritePath);

    if (m_sheet.pixmap.isNull()) {
        qFatal("Could not load resources/sprites/player.png");
    }

    // set up layout — no margins so the view fills the whole widget
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // set up the graphics scene and view
    m_scene = new QGraphicsScene(0, 0, WORLD_W, WORLD_H, this);

    m_view = new QGraphicsView(m_scene, this);
    m_view->setFrameShape(QFrame::NoFrame);
    m_view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_view->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(m_view);

    // game loop timer - fires every 16ms (~60fps)
    m_ticker.setInterval(16);
    connect(&m_ticker, &QTimer::timeout, this, &OverworldWidget::onTick);

    buildScene();
    setFocusPolicy(Qt::StrongFocus);
    m_view->setSceneRect(0, 0, WORLD_W, WORLD_H);

    buildPauseOverlay();

    // gold HUD goes in the top-right corner
    m_goldHud = new GoldHudWidget(this);
    m_goldHud->raise();
}


// ============================================================
//  OverworldWidget  — activate / deactivate
// ============================================================

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


// ============================================================
//  OverworldWidget  — scene construction
//  This builds the whole map: grass, paths, house, trees, dungeon door
// ============================================================

void OverworldWidget::buildScene()
{
    // --- grass background tiles ---
    QPixmap grassTile(":/sprites/grass.png");

    const int TILE_SIZE = 64;

    QPixmap scaledTile = grassTile.scaled(
        TILE_SIZE, TILE_SIZE,
        Qt::IgnoreAspectRatio,
        Qt::SmoothTransformation
    );

    for (int y = 0; y < WORLD_H; y += TILE_SIZE) {
        for (int x = 0; x < WORLD_W; x += TILE_SIZE) {
            QGraphicsPixmapItem *tile = m_scene->addPixmap(scaledTile);
            tile->setPos(x, y);
            tile->setZValue(0);
        }
    }

    // --- dirt path down the middle ---
    QPixmap dirt1(":/sprites/dirt1.png");
    QPixmap dirt2(":/sprites/dirt2.png");

    const int TILE = 64;

    QPixmap d1 = dirt1.scaled(TILE, TILE, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    QPixmap d2 = dirt2.scaled(TILE, TILE, Qt::IgnoreAspectRatio, Qt::FastTransformation);

    qreal pathX = WORLD_W * 0.38;
    qreal pathW = WORLD_W * 0.24;

    for (int y = 0; y < WORLD_H; y += TILE) {
        for (int x = pathX; x < pathX + pathW; x += TILE) {
            // randomly use the alternate dirt tile to add some variety
            bool useAlt = (QRandomGenerator::global()->bounded(100) < 24);
            QGraphicsPixmapItem *tile = m_scene->addPixmap(useAlt ? d2 : d1);
            tile->setPos(x, y);
            tile->setZValue(1);
        }
    }

    // --- house ---
    static constexpr int HOUSE_W = 240;
    static constexpr int HOUSE_H = 240;
    static constexpr int HOUSE_X = 60;
    static constexpr int HOUSE_Y = 180;

    {
        // shadow under the house
        const qreal shadowW = HOUSE_W * 0.75;
        const qreal shadowH = HOUSE_H * 0.30;

        auto *houseShadow = m_scene->addRect(
            0, 0, shadowW, shadowH,
            Qt::NoPen, QBrush(QColor(0, 0, 0, 65))
        );
        houseShadow->setPos(HOUSE_X + 33, HOUSE_Y + 155);
        houseShadow->setZValue(2);
    }

    QPixmap housePx(":/sprites/house.png");
    if (!housePx.isNull()) {
        QGraphicsPixmapItem *houseItem = m_scene->addPixmap(
            housePx.scaled(HOUSE_W, HOUSE_H, Qt::KeepAspectRatio, Qt::FastTransformation)
        );
        houseItem->setPos(HOUSE_X, HOUSE_Y);
        houseItem->setZValue(10);
    } else {
        qWarning("Could not load resources/sprites/house.png");
    }

    // invisible collision box for the house body (so player cant walk through it)
    const qreal colliderInset = HOUSE_W * 0.10;
    const qreal colliderY     = HOUSE_Y + HOUSE_H * 0.60;
    const qreal colliderH     = HOUSE_H * 0.25;
    m_houseCollider = m_scene->addRect(
        HOUSE_X + colliderInset, colliderY,
        HOUSE_W - colliderInset * 2, colliderH,
        Qt::NoPen, Qt::NoBrush
    );
    m_houseCollider->setZValue(1);


    // ── House entrance zone (triggers transition to house screen) ────────────
    const qreal entranceW = HOUSE_W * 0.35;
    const qreal entranceX = HOUSE_X + (HOUSE_W - entranceW) / 2.0;
    const qreal entranceY = HOUSE_Y + HOUSE_H * 0.82;
    m_houseEntranceZone = m_scene->addRect(
        entranceX, entranceY, entranceW, 20,
        Qt::NoPen, Qt::NoBrush);
    m_houseEntranceZone->setZValue(1);
    // ── Shop zone ────────────────────────────────────────────────────────────────
    m_shopZone = m_scene->addRect(
    620, 220, 80, 60,
    QPen(Qt::NoPen),
    QBrush(QColor(255, 215, 0, 80))
    );
    m_shopZone->setZValue(2);

    auto *shopLabel = m_scene->addText("SHOP", QFont("Arial", 8, QFont::Bold));
    shopLabel->setDefaultTextColor(QColor("#FFD700"));
    shopLabel->setPos(628, 198);
    shopLabel->setZValue(3);
    // ── Trees ────────────────────────────────────────────────────────────────
    QPixmap treeRoundPx(":/sprites/tree_round.png");
    QPixmap treePinePx (":/sprites/tree_pine.png");

    const int TREE_W = 160;
    const int TREE_H = 160;
    QPixmap treeRound = treeRoundPx.scaled(TREE_W, TREE_H, Qt::KeepAspectRatio, Qt::FastTransformation);
    QPixmap treePine  = treePinePx .scaled(TREE_W, TREE_H, Qt::KeepAspectRatio, Qt::FastTransformation);

    // position + which type of tree (true = round, false = pine)
    const QList<QPair<QPointF, bool>> trees = {
        {{60,  60},  false},
        {{150, 50},  true },
        {{680, 55},  true },
        {{740, 130}, false},
        {{60,  430}, true },
        {{130, 510}, false},
        {{690, 420}, false},
        {{750, 300}, true },
        {{550, 480}, true },
        {{580, 80},  false},
    };

    for (const auto &[p, isRound] : trees) {
        const QPixmap &px = isRound ? treeRound : treePine;
        const qreal tw = px.width();
        const qreal th = px.height();

        // shadow ellipse under the trunk
        const qreal shadowW = tw * 0.65;
        const qreal shadowH = th * 0.15;
        const qreal shadowX = p.x() + (tw - shadowW) / 2.0;
        const qreal shadowY = p.y() + th - shadowH * 0.8;

        auto *shadow = m_scene->addEllipse(
            shadowX, shadowY, shadowW, shadowH,
            Qt::NoPen, Qt::NoBrush
        );

        QRadialGradient grad(
            shadowX + shadowW / 2.0,
            shadowY + shadowH / 2.0,
            shadowW / 2.0
        );
        grad.setColorAt(0.0, QColor(0, 0, 0, 120));
        grad.setColorAt(0.6, QColor(0, 0, 0, 50));
        grad.setColorAt(1.0, QColor(0, 0, 0, 0));

        shadow->setBrush(grad);
        shadow->setZValue(2);

        // tree sprite on top of the shadow
        auto *treeItem = m_scene->addPixmap(px);
        treeItem->setPos(p.x(), p.y());
        treeItem->setZValue(3);
    }

    // --- dungeon entrance at the top ---
    const qreal dw = 80, dh = 56;
    const qreal dx = (WORLD_W - dw) / 2.0;
    const qreal dy = 6;

    // dark stone border around the entrance
    m_scene->addRect(
        dx - 10, dy - 6, dw + 20, dh + 10,
        Qt::NoPen, QBrush(QColor("#424242"))
    )->setZValue(1);

    // the zone that triggers dungeon entry
    m_dungeonZone = m_scene->addRect(dx, dy, dw, dh, Qt::NoPen, QBrush(QColor("#1a1a2e")));
    m_dungeonZone->setZValue(2);

    auto *skull = m_scene->addText("☠  DUNGEON", QFont("Arial", 8, QFont::Bold));
    skull->setDefaultTextColor(QColor("#ef5350"));
    skull->setPos(dx + 2, dy + dh + 4);
    skull->setZValue(3);

    // --- controls hint at the bottom ---
    auto *hint = m_scene->addText(
        "WASD / Arrow keys to move    ESC = menu",
        QFont("Arial", 7)
    );
    hint->setDefaultTextColor(QColor("#ffffffaa"));
    hint->setPos(8, WORLD_H - 20);
    hint->setZValue(10);

    // --- player sprite ---
    m_player = new PlayerSprite(m_sheet);
    m_player->setZValue(9);
    m_scene->addItem(m_player);
    placePlayer();
}

// put the player in the center of the map
void OverworldWidget::placePlayer()
{
    if (m_player) {
        m_player->setPos(
            (WORLD_W - PlayerSprite::W) / 2.0,
            (WORLD_H - PlayerSprite::H) / 2.0
        );
    }
}


// ============================================================
//  OverworldWidget  — game loop
// ============================================================

// called every tick by the timer
void OverworldWidget::onTick()
{
    m_player->step(m_heldKeys, QRectF(0, 0, WORLD_W, WORLD_H));
    checkTriggers();
}

// check if the player has walked into any trigger zones
void OverworldWidget::checkTriggers()
{
    // dungeon entrance — teleport player to dungeon screen
    if (m_player->collidesWithItem(m_dungeonZone)) {
        deactivate();
        emit dungeonEntered();
        return;
    }

    // house collision — push player out so they cant walk through the wall
    if (m_houseCollider && m_player->collidesWithItem(m_houseCollider)) {
        const QRectF playerRect   = m_player->mapToScene(m_player->boundingRect()).boundingRect();
        const QRectF colliderRect = m_houseCollider->sceneBoundingRect();

        // how much overlap on each side
        const qreal overlapLeft  = playerRect.right()   - colliderRect.left();
        const qreal overlapRight = colliderRect.right()  - playerRect.left();
        const qreal overlapTop   = playerRect.bottom()   - colliderRect.top();
        const qreal overlapBot   = colliderRect.bottom() - playerRect.top();

        const qreal minX = qMin(overlapLeft,  overlapRight);
        const qreal minY = qMin(overlapTop,   overlapBot);

        qreal px = m_player->x();
        qreal py = m_player->y();

        // push out on the axis with least overlap
        if (minX < minY) {
            px += (overlapLeft < overlapRight) ? -overlapLeft : overlapRight;
        } else {
            py += (overlapTop  < overlapBot)   ? -overlapTop  : overlapBot;
        }

        m_player->setPos(px, py);
    }
    // ── House entrance → enter house screen ──────────────────────────────────
    if (m_houseEntranceZone && m_player->collidesWithItem(m_houseEntranceZone)) {
        deactivate();
        emit houseEntered();
        return;
    }
   //--shop------------------------------------------------------------------ 
    if (m_shopZone && m_player->collidesWithItem(m_shopZone)) {
    deactivate();
    emit shopEntered();
    return;
}
}


// ============================================================
//  OverworldWidget  — input handling
// ============================================================

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


// ============================================================
//  OverworldWidget  — resize / show events
// ============================================================

void OverworldWidget::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);

    if (m_pauseOverlay)
        m_pauseOverlay->resize(size());

    // keep gold HUD pinned to the top-right corner
    if (m_goldHud)
        m_goldHud->move(width() - m_goldHud->width() - 8, 8);

    fitView();
}

void OverworldWidget::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    fitView();
}

// scale the view so the whole world fits inside the widget
void OverworldWidget::fitView()
{
    if (!m_view) return;
    m_view->fitInView(0, 0, WORLD_W, WORLD_H, Qt::KeepAspectRatio);
}


// ============================================================
//  OverworldWidget  — pause menu
// ============================================================

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

// build the pause overlay widget (only called once in the constructor)
void OverworldWidget::buildPauseOverlay()
{
    m_pauseOverlay = new QWidget(this);
    m_pauseOverlay->setAttribute(Qt::WA_TranslucentBackground);
    m_pauseOverlay->hide();

    auto *layout = new QVBoxLayout(m_pauseOverlay);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(16);

    auto *title  = new QLabel("— PAUSED —",   m_pauseOverlay);
    auto *resume = new QPushButton("► RESUME",    m_pauseOverlay);
    auto *save   = new QPushButton("  SAVE GAME", m_pauseOverlay);
    auto *menu   = new QPushButton("  MAIN MENU", m_pauseOverlay);

    title->setObjectName("titleLabel");
    title->setAlignment(Qt::AlignCenter);

    connect(resume, &QPushButton::clicked, this, &OverworldWidget::togglePause);

    connect(save, &QPushButton::clicked, this, [this] {
        emit saveRequested();
    });

    connect(menu, &QPushButton::clicked, this, [this] {
        m_paused = false;
        m_pauseOverlay->hide();
        deactivate();
        emit backToMenu();
    });

    layout->addWidget(title);
    layout->addSpacing(12);
    layout->addWidget(resume);
    layout->addWidget(save);
    layout->addWidget(menu);
}


// ============================================================
//  OverworldWidget  — misc public methods
// ============================================================

void OverworldWidget::setGold(int gold)
{
    if (m_goldHud) m_goldHud->setGold(gold);
}

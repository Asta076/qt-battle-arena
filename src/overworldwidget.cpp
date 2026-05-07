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
#include "pauseoverlaywidget.h"
#include "spritecache.h"


// ============================================================
//  PlayerSprite  — constructor
// ============================================================

PlayerSprite::PlayerSprite(const SpriteSheet &sheet, QGraphicsItem *parent)
    : QGraphicsPixmapItem(parent)
    , m_sheet(sheet) {
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

void PlayerSprite::updateAnimation(bool isMoving, Direction facingDir)
{
    if (isMoving) {
        setWalkAnim(facingDir);
        ++m_tickAccum;
        if (m_tickAccum >= TICKS_PER_FRAME) {
            m_tickAccum  = 0;
            m_frameIndex = (m_frameIndex + 1) % WALK_FRAMES;
            applyFrame();
        }
    } else {
        if (!m_isIdle) setIdleFrame(m_facing);
    }
}

// ============================================================
//  OverworldWidget  — constructor
// ============================================================

OverworldWidget::OverworldWidget(AudioManager *audio, QWidget *parent)
    : QWidget(parent)
    , m_audio(audio)
{
    // load the player sprite sheet from resources
    m_sheet.pixmap = SpriteCache::instance().get(":/sprites/player.png");

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

    m_pauseOverlay = new PauseOverlayWidget(true, this);  // true = show save
    connect(m_pauseOverlay, &PauseOverlayWidget::resumeRequested, this, &OverworldWidget::togglePause);
    connect(m_pauseOverlay, &PauseOverlayWidget::saveRequested, this, &OverworldWidget::saveRequested);
    connect(m_pauseOverlay, &PauseOverlayWidget::menuRequested, this, [this] {
    m_paused = false;
    m_pauseOverlay->hide();
    deactivate();
    emit backToMenu();
});

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
        houseItem->setZValue(5);
    } else {
        qWarning("Could not load resources/sprites/house.png");
    }

    // ── House solid collider ─────────────────────────────────────────────────
    const qreal colliderInset = HOUSE_W * 0.20;           // 48px each side
    const qreal colliderY     = HOUSE_Y + HOUSE_H * 0.82; // Y=348 — just above door frame
    const qreal colliderH     = HOUSE_H - HOUSE_H * 0.82; // extends to Y=420, house bottom
    m_houseCollider = m_scene->addRect(
        HOUSE_X + colliderInset, colliderY,
        HOUSE_W - colliderInset * 2, colliderH,
        Qt::NoPen, Qt::NoBrush
        );
    m_houseCollider->setZValue(1);

    // Entrance zone: narrow strip at TOP of the collider (player approaches from below)
    const qreal doorW = HOUSE_W * 0.22;
    const qreal doorX = HOUSE_X + (HOUSE_W - doorW) / 2.0;
    const qreal doorY = colliderY + 2.0;                        // flush with collider top
    m_houseEntranceZone = m_scene->addRect(
        doorX, doorY, doorW, 16,
        Qt::NoPen, Qt::NoBrush);
    m_houseEntranceZone->setZValue(1);
    // ── Shop ────────────────────────────────────────────────────────────────
    const qreal SHOP_X = 600;
    const qreal SHOP_Y = 165;
    const qreal SHOP_W = 120;
    const qreal SHOP_H = 95;

    // 1. Load the shop image from your resources
    QPixmap shopPixmap(":/sprites/shop.png");

    // FIX: Scale the image to perfectly match SHOP_W and SHOP_H
    // Qt::KeepAspectRatio ensures the image doesn't look stretched or squished.
    // If it still doesn't fit your box perfectly, change Qt::KeepAspectRatio to Qt::IgnoreAspectRatio
    shopPixmap = shopPixmap.scaled(SHOP_W, SHOP_H, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // Add the scaled image to the scene
    QGraphicsPixmapItem* shopItem = m_scene->addPixmap(shopPixmap);
    shopItem->setPos(SHOP_X, SHOP_Y);
    shopItem->setZValue(4); // Sets it above the ground, adjust if needed

    // 2. Add the physical collision box (Collider)
    QGraphicsRectItem* shopCollider = m_scene->addRect(
        SHOP_X, SHOP_Y, SHOP_W, SHOP_H,
        Qt::NoPen, Qt::NoBrush // Keep it invisible
        );
    // Optional: If your game uses data tags to identify solid objects
    // shopCollider->setData(0, "Solid");

    // 3. Invisible trigger zone in front of the stall
    m_shopZone = m_scene->addRect(
        SHOP_X + 20, SHOP_Y + SHOP_H + 2, SHOP_W - 40, 18,
        Qt::NoPen,
        Qt::NoBrush
        );
    m_shopZone->setZValue(1);
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

    // ── Level 1 entrance — left of the house ─────────────────────────────────
    static constexpr qreal L1_W = 72;
    static constexpr qreal L1_H = 52;
    static constexpr qreal L1_X = HOUSE_X + (HOUSE_W - L1_W) / 2.0;  // centered under house
    static constexpr qreal L1_Y = HOUSE_Y + HOUSE_H + 90;

    // stone border
    m_scene->addRect(
               L1_X - 8, L1_Y - 6, L1_W + 16, L1_H + 10,
               Qt::NoPen, QBrush(QColor("#424242"))
               )->setZValue(1);

    // the dark portal / gate
    m_level1Zone = m_scene->addRect(
        L1_X, L1_Y, L1_W, L1_H,
        Qt::NoPen, QBrush(QColor("#1a1a2e"))
        );
    m_level1Zone->setZValue(2);

    // label
    auto *lvl1Label = m_scene->addText("⚔  LEVEL 1", QFont("Arial", 8, QFont::Bold));
    lvl1Label->setDefaultTextColor(QColor("#FFD700"));
    lvl1Label->setPos(L1_X - 4, L1_Y + L1_H + 4);
    lvl1Label->setZValue(3);
    // --- player sprite ---
    m_player = new PlayerSprite(m_sheet);
    m_player->setZValue(9);
    m_scene->addItem(m_player);
    placePlayer();


    drawDebugColliders();
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
    QPointF vel = m_controller.computeVelocity(m_heldKeys);

    // ── Feet rect — only the bottom 20px of the sprite is used for collision.
    // This makes the player feel like they're touching what they look like
    // they're touching, instead of their 96px bounding box hitting first.
    auto feetRect = [&](qreal px, qreal py) -> QRectF {
        const qreal feetH = 20.0;
        return QRectF(
            px + PlayerSprite::W * 0.25,           // inset sides 25% each
            py + PlayerSprite::H - feetH,           // bottom strip only
            PlayerSprite::W * 0.50,                 // 50% of sprite width
            feetH
            );
    };

    qreal nx = qBound(0.0, m_player->x() + vel.x(), WORLD_W - PlayerSprite::W);
    qreal ny = qBound(0.0, m_player->y() + vel.y(), WORLD_H - PlayerSprite::H);

    if (m_houseCollider) {
        QRectF solid = m_houseCollider->sceneBoundingRect();

        // Resolve X independently using current Y
        if (feetRect(nx, m_player->y()).intersects(solid)) {
            nx = (vel.x() > 0)
            ? solid.left()  - PlayerSprite::W * 0.75   // snap left edge of feet
            : solid.right() - PlayerSprite::W * 0.25;  // snap right edge of feet
            nx = qBound(0.0, nx, WORLD_W - PlayerSprite::W);
        }

        if (feetRect(m_player->x(), ny).intersects(solid)) {
            if (vel.y() > 0)
                ny = solid.top() - PlayerSprite::H + 20.0;
            else
                ny = solid.bottom() - PlayerSprite::H + 20.0;
            ny = qBound(0.0, ny, WORLD_H - PlayerSprite::H);
        }
    }

    m_player->setPos(nx, ny);

    bool moving = m_controller.isMoving(m_heldKeys);
    Direction dir = m_controller.computeDirection(m_heldKeys);
    m_player->updateAnimation(moving, dir);

    checkTriggers();
}

// check if the player has walked into any trigger zones
void OverworldWidget::checkTriggers()
{
    // Feet rect — matches onTick so triggers fire at the same point walls stop
    const qreal feetH = 20.0;
    QRectF feet(
        m_player->x() + PlayerSprite::W * 0.25,
        m_player->y() + PlayerSprite::H - feetH,
        PlayerSprite::W * 0.50,
        feetH
        );

    if (m_dungeonZone && feet.intersects(m_dungeonZone->sceneBoundingRect())) {
        deactivate();
        emit dungeonEntered();
        return;
    }

    // House entrance — player must press W/Up while feet touch the door strip
    if (m_houseEntranceZone
        && feet.intersects(m_houseEntranceZone->sceneBoundingRect())) {
        if (m_heldKeys.contains(Qt::Key_W) || m_heldKeys.contains(Qt::Key_Up)) {
            deactivate();
            emit houseEntered();
            return;
        }
    }

    if (m_shopZone && feet.intersects(m_shopZone->sceneBoundingRect())) {
        deactivate();
        emit shopEntered();
        return;
    }

    if (m_level1Zone && feet.intersects(m_level1Zone->sceneBoundingRect())) {
        deactivate();
        emit level1Entered();
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
    m_view->fitInView(0, 0, WORLD_W, WORLD_H, Qt::IgnoreAspectRatio);
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


// ============================================================
//  OverworldWidget  — misc public methods
// ============================================================

void OverworldWidget::setGold(int gold)
{
    if (m_goldHud) m_goldHud->setGold(gold);
}

void OverworldWidget::drawDebugColliders()
{
    if (!DEBUG_COLLIDERS) return;

    auto highlight = [&](QGraphicsRectItem* item, QColor color) {
        if (!item) return;
        item->setPen(QPen(color, 2));
        item->setBrush(QBrush(QColor(color.red(), color.green(), color.blue(), 40)));
        item->setZValue(99);  // always on top
    };

    highlight(m_houseCollider,     QColor("#FF4444"));  // red   = solid wall
    highlight(m_houseEntranceZone, QColor("#44FF44"));  // green = door trigger
    highlight(m_dungeonZone,       QColor("#4444FF"));  // blue  = dungeon trigger
    highlight(m_shopZone,          QColor("#FFAA00"));  // orange = shop trigger
    highlight(m_level1Zone,        QColor("#FF44FF"));  // pink  = level 1 trigger
}

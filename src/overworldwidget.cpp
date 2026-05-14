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
void PlayerSprite::refreshFrame()
{
    applyFrame();
}

// ============================================================
//  OverworldWidget  — constructor
// ============================================================

OverworldWidget::OverworldWidget(AudioManager *audio, QWidget *parent)
    : QWidget(parent)
    , m_audio(audio)
{
    // load the player sprite sheet from resources
    loadPlayerSheet(m_playerType);

    // set up layout — no margins so the view fills the whole widget
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // set up the graphics scene and view
    m_scene = new QGraphicsScene(0, 0, WORLD_W, WORLD_H, this);

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
void OverworldWidget::loadPlayerSheet(CharacterType type)
{
    m_sheet.pixmap = SpriteCache::instance().get(movementSheetFor(type));

    // Fallback just in case the new file path is wrong.
    if (m_sheet.pixmap.isNull()) {
        m_sheet.pixmap = SpriteCache::instance().get(":/sprites/player.png");
    }
}

void OverworldWidget::setPlayerCharacterType(CharacterType type)
{
    m_playerType = type;
    loadPlayerSheet(type);

    if (m_player) {
        m_player->refreshFrame();
    }
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
    m_treeCollisionRects.clear();

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

    // Keep the path based on the original 800px layout.
    // WORLD_W is wider now only to add extra grass on the right.
    qreal pathX = BASE_WORLD_W * 0.38;
    qreal pathW = BASE_WORLD_W * 0.24;

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


    const qreal colliderInset = HOUSE_W * 0.05;
    const qreal colliderY     = HOUSE_Y + HOUSE_H * 0.45;  // starts higher up
    const qreal colliderH     = HOUSE_H * 0.50;             // taller — covers more
    m_houseCollider = m_scene->addRect(
        HOUSE_X + colliderInset, colliderY,
        HOUSE_W - colliderInset * 2, colliderH,
        Qt::NoPen, Qt::NoBrush
        );
    m_houseCollider->setZValue(1);


    const qreal entranceW = HOUSE_W * 0.30;
    const qreal entranceX = HOUSE_X + (HOUSE_W - entranceW) / 2.0;
    const qreal entranceY = colliderY + colliderH + 2.0;   // flush below collider
    m_houseEntranceZone = m_scene->addRect(
        entranceX, entranceY, entranceW, 16,
        Qt::NoPen, Qt::NoBrush);
    m_houseEntranceZone->setZValue(1);
    // ── Shop ────────────────────────────────────────────────────────────────
    const qreal SHOP_X = 450;
    const qreal SHOP_Y = 220;
    const qreal SHOP_W = 270;
    const qreal SHOP_H = 360;

    // Shadow under the shop
    const qreal shopShadowW = SHOP_W * 0.85;
    const qreal shopShadowH = SHOP_H * 0.28;
    const qreal shopShadowX = SHOP_X + (SHOP_W - shopShadowW) / 2.0;
    const qreal shopShadowY = SHOP_Y + SHOP_H - shopShadowH * 0.45;

    QGraphicsRectItem* shopShadow = m_scene->addRect(
        shopShadowX+30,
        shopShadowY-210,
        shopShadowW-40,
        shopShadowH-20,
        Qt::NoPen,
        QBrush(QColor(0, 0, 0, 65))
    );

    shopShadow->setZValue(2);

    QPixmap shopPixmap(":/sprites/shop.png");


    shopPixmap = shopPixmap.scaled(SHOP_W, SHOP_H, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // Add the scaled image to the scene
    QGraphicsPixmapItem* shopItem = m_scene->addPixmap(shopPixmap);
    shopItem->setPos(SHOP_X, SHOP_Y);
    shopItem->setZValue(10);


    QGraphicsRectItem* shopCollider = m_scene->addRect(
        SHOP_X, SHOP_Y, SHOP_W, SHOP_H,
        Qt::NoPen, Qt::NoBrush // Keep it invisible
        );

    m_shopZone = m_scene->addRect(
    SHOP_X + 77,
    SHOP_Y + 105,
    SHOP_W -160,
    SHOP_H-280,
    Qt::NoPen,
    QBrush(Qt::transparent)
);

    m_shopZone->setZValue(20);


    m_shopCollider = m_scene->addRect(
    SHOP_X + 57,
    SHOP_Y + 105,
    SHOP_W -120,
    SHOP_H - 330,
    Qt::NoPen,
    QBrush(Qt::transparent)
);

    m_shopCollider->setZValue(20);
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
        {{640, 130}, false},
        {{60,  430}, true },
        {{130, 510}, false},
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

        // Tree visual goes above the player so the player can appear behind it.
        auto *treeItem = m_scene->addPixmap(px);
        treeItem->setPos(p.x(), p.y());
        treeItem->setZValue(12);

        // Collision is only the lower trunk/base area, not the whole tree.
        // Store it as plain data instead of an invisible QGraphicsRectItem.
        qreal colliderX;
        qreal colliderY;
        qreal colliderW;
        qreal colliderH;

        if (isRound) {
            colliderX = p.x() + tw * 0.36;
            colliderY = p.y() + th * 0.72;
            colliderW = tw * 0.28;
            colliderH = th * 0.16;
        } else {
            colliderX = p.x() + tw * 0.34;
            colliderY = p.y() + th * 0.74;
            colliderW = tw * 0.32;
            colliderH = th * 0.15;
        }

        m_treeCollisionRects.append(QRectF(
            colliderX,
            colliderY,
            colliderW,
            colliderH
        ));
    }

    // --- dungeon entrance ---
    static constexpr int DUNGEON_W = 384;
    static constexpr int DUNGEON_H = 384;
    static constexpr int DUNGEON_X = 230;
    static constexpr int DUNGEON_Y = -175;

    // Shadow underneath cave image
    QGraphicsEllipseItem* caveShadow = m_scene->addEllipse(
        DUNGEON_X + 54,
        DUNGEON_Y + 125,
        DUNGEON_W - 140,
        DUNGEON_H-225,
        Qt::NoPen,
        QBrush(QColor(0, 0, 0, 120))
    );

    caveShadow->setZValue(5);

    // Visible dungeon entrance image
    QPixmap dungeonPx(":/sprites/dungeon_entrance.png");

    if (!dungeonPx.isNull()) {
        QGraphicsPixmapItem* dungeonItem = m_scene->addPixmap(
            dungeonPx.scaled(
                DUNGEON_W,
                DUNGEON_H,
                Qt::KeepAspectRatio,
                Qt::FastTransformation
            )
        );

        dungeonItem->setPos(DUNGEON_X, DUNGEON_Y);
        dungeonItem->setZValue(6);
    } else {
        qWarning("Could not load :/sprites/dungeon_entrance.png");
    }

    // Invisible trigger zone.
    // Keep this because checkTriggers() already uses m_dungeonZone.
    m_dungeonZone = m_scene->addRect(
        DUNGEON_X + 28,
        DUNGEON_Y + 76,
        DUNGEON_W - 56,
        DUNGEON_H-225,
        Qt::NoPen,
        QBrush(Qt::red)
    );

    m_dungeonZone->setZValue(20);
    m_dungeonZone->setVisible(false);

    // --- controls hint at the bottom ---
    auto *hint = m_scene->addText(
        "WASD / Arrow keys to move    ESC = menu",
        QFont("Arial", 7)
    );
    hint->setDefaultTextColor(QColor("#ffffffaa"));
    hint->setPos(8, WORLD_H - 20);
    hint->setZValue(10);

// ── LEVEL SELECTOR ─────────────────────────────────────────────────────


static constexpr qreal LEVEL_W = 320.0;
static constexpr qreal LEVEL_H = 320.0;


const qreal LEVEL_X = 735.0;
const qreal LEVEL_Y = 5.0;


const qreal levelShadowW = LEVEL_W * 0.58;
const qreal levelShadowH = LEVEL_H * 0.20;
const qreal levelShadowX = LEVEL_X + (LEVEL_W - levelShadowW) / 2.0;
const qreal levelShadowY = LEVEL_Y + LEVEL_H * 0.66;

auto* levelShadow = m_scene->addRect(
    levelShadowX,
    levelShadowY,
    levelShadowW,
    levelShadowH,
    Qt::NoPen,
    QBrush(QColor(0, 0, 0, 75))
);
levelShadow->setZValue(2);


QPixmap levelSelectorPx(":/sprites/level_selector.png");

if (!levelSelectorPx.isNull()) {
    QGraphicsPixmapItem* levelSelectorItem = m_scene->addPixmap(
        levelSelectorPx.scaled(
            static_cast<int>(LEVEL_W),
            static_cast<int>(LEVEL_H),
            Qt::KeepAspectRatio,
            Qt::FastTransformation
        )
    );

    levelSelectorItem->setPos(LEVEL_X, LEVEL_Y);


    levelSelectorItem->setZValue(12);
} else {
    qWarning("Could not load :/sprites/level_selector.png");
}

const qreal triggerInsetX = LEVEL_W * 0.28;
const qreal triggerX      = LEVEL_X + triggerInsetX;
const qreal triggerY      = LEVEL_Y + LEVEL_H * 0.54;
const qreal triggerW      = LEVEL_W - triggerInsetX * 2.0;
const qreal triggerH      = LEVEL_H * 0.20;

m_level1Zone = m_scene->addRect(
    triggerX,
    triggerY,
    triggerW,
    triggerH,
    Qt::NoPen,
    Qt::NoBrush
);

m_level1Zone->setZValue(20);
m_level1Zone->setVisible(false);

    // --- player sprite ---
    m_player = new PlayerSprite(m_sheet);
    m_player->setZValue(9);
    m_scene->addItem(m_player);
    placePlayer();
}

void OverworldWidget::placePlayer()
{
    if (!m_player)
        return;

    const qreal pathX = BASE_WORLD_W * 0.38;
    const qreal pathW = BASE_WORLD_W * 0.24;

    const qreal spawnX = pathX + pathW / 2.0 - PlayerSprite::W / 2.0;
    const qreal spawnY = WORLD_H / 2.0 - PlayerSprite::H / 2.0;

    m_player->setPos(spawnX, spawnY);
}


// ============================================================
//  OverworldWidget  — game loop
// ============================================================

// called every tick by the timer
void OverworldWidget::onTick()
{
    if (!m_player || m_paused)
        return;

    QList<QRectF> solids;

    if (m_houseCollider)
        solids.append(m_houseCollider->sceneBoundingRect());

    if (m_shopCollider)
        solids.append(m_shopCollider->sceneBoundingRect());

    solids += m_treeCollisionRects;

    OverworldLogicManager::MovementResult movement = m_logic.resolvePlayerMovement(
        m_player->pos(),
        m_heldKeys,
        PlayerSprite::SPEED,
        PlayerSprite::W,
        PlayerSprite::H,
        QRectF(0, 0, WORLD_W, WORLD_H),
        solids,
        Direction::Down
    );

    m_player->setPos(movement.position);
    m_player->updateAnimation(movement.moving, movement.facing);

    checkTriggers();
}

// check if the player has walked into any trigger zones
void OverworldWidget::checkTriggers()
{
    if (!m_player)
        return;

    OverworldLogicManager::Trigger trigger = m_logic.triggerFor(
        m_player->sceneBoundingRect(),
        m_dungeonZone ? m_dungeonZone->sceneBoundingRect() : QRectF(),
        m_houseEntranceZone ? m_houseEntranceZone->sceneBoundingRect() : QRectF(),
        m_shopZone ? m_shopZone->sceneBoundingRect() : QRectF(),
        m_level1Zone ? m_level1Zone->sceneBoundingRect() : QRectF(),
        m_heldKeys
    );

    switch (trigger) {
    case OverworldLogicManager::Trigger::Dungeon:
        deactivate();
        emit dungeonEntered();
        return;

    case OverworldLogicManager::Trigger::House:
        deactivate();
        emit houseEntered();
        return;

    case OverworldLogicManager::Trigger::Shop:
        deactivate();
        emit shopEntered();
        return;

    case OverworldLogicManager::Trigger::Levels:
        deactivate();
        emit levelsEntered();
        return;

    case OverworldLogicManager::Trigger::None:
        return;
    }
}

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


// ============================================================
//  OverworldWidget  — misc public methods
// ============================================================

void OverworldWidget::setGold(int gold)
{
    if (m_goldHud) m_goldHud->setGold(gold);
}



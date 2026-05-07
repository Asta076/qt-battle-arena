#include "level1widget.h"
#include "ui_level1widget.h"
#include "audiomanager.h"
#include "spritecache.h"
#include "pauseoverlaywidget.h"

// ── Qt ────────────────────────────────────────────────────────────────────────
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QRadialGradient>
#include <QPainter>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QFont>
#include <QRandomGenerator>

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

Level1Widget::Level1Widget(AudioManager* audio, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::Level1Widget)
    , m_audio(audio)
{
    ui->setupUi(this);

    // Load shared player sprite sheet (same asset as overworld / dungeon)
    m_sheet.pixmap = SpriteCache::instance().get(":/sprites/player.png");

    // Wire the QGraphicsView that came from the .ui file
    m_view  = ui->graphicsView;
    m_scene = new QGraphicsScene(0, 0, WORLD_W, WORLD_H, this);
    m_view->setScene(m_scene);
    m_view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_view->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing);
    m_view->setFocusPolicy(Qt::NoFocus);

    // Game loop — ~60 fps
    m_ticker.setInterval(16);
    connect(&m_ticker, &QTimer::timeout, this, &Level1Widget::onTick);

    buildScene();

    setFocusPolicy(Qt::StrongFocus);
    m_view->setSceneRect(0, 0, WORLD_W, WORLD_H);

    // Pause overlay — showSave=false (no mid-level saves, same as dungeon)
    m_pauseOverlay = new PauseOverlayWidget(false, this);
    connect(m_pauseOverlay, &PauseOverlayWidget::resumeRequested,
            this, &Level1Widget::togglePause);
    connect(m_pauseOverlay, &PauseOverlayWidget::menuRequested, this, [this] {
        m_paused = false;
        m_pauseOverlay->hide();
        deactivate();
        emit backToMenu();
    });

    // Gold HUD — pinned to top-right in resizeEvent
    m_goldHud = new GoldHudWidget(this);
    m_goldHud->raise();
}

Level1Widget::~Level1Widget()
{
    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────────────────────────────────────

void Level1Widget::activate(const LevelDef& level, const PlayerProfile& profile)
{
    m_level          = level;
    m_bossTriggered  = false;
    m_bossSprite     = nullptr;

    if (m_audio) m_audio->playMusic("/music/overworld.ogg");
    m_heldKeys.clear();

    m_scene->clear();
    m_player = nullptr;
    m_bossSprite = nullptr;
    m_enemies.clear();
    buildScene();

    placePlayer();
    spawnEnemies();    // spawns regular enemies AND the boss
    m_goldHud->setGold(profile.gold);
    m_ticker.start();
    setFocus();
}

void Level1Widget::deactivate()
{
    m_ticker.stop();
    m_heldKeys.clear();
}

void Level1Widget::setGold(int gold)
{
    if (m_goldHud) m_goldHud->setGold(gold);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Scene construction  —  Forest Path visual theme
// ─────────────────────────────────────────────────────────────────────────────

void Level1Widget::buildScene()
{
     const int TILE = 64;
    QPixmap grassTile = SpriteCache::instance().get(":/sprites/grass.png")
                            .scaled(TILE, TILE, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    for (int y = 0; y < WORLD_H; y += TILE) {
        for (int x = 0; x < WORLD_W; x += TILE) {
            auto* tile = m_scene->addPixmap(grassTile);
            tile->setPos(x, y);
            tile->setZValue(0);
        }
    }

    QPixmap d1 = SpriteCache::instance().get(":/sprites/dirt1.png")
                     .scaled(TILE, TILE, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    QPixmap d2 = SpriteCache::instance().get(":/sprites/dirt2.png")
                     .scaled(TILE, TILE, Qt::IgnoreAspectRatio, Qt::FastTransformation);

    auto* rng = QRandomGenerator::global();
    const int pathX = 340, pathW = 128;   // 2 tiles wide
    for (int y = 0; y < WORLD_H; y += TILE) {
        for (int x = pathX; x < pathX + pathW; x += TILE) {
            bool alt = rng->bounded(100) < 24;
            auto* tile = m_scene->addPixmap(alt ? d2 : d1);
            tile->setPos(x, y);
            tile->setZValue(1);
        }
    }

    // ── Trees — uses the repo's existing tree sprites ─────────────────────────
    const int TREE_W = 160, TREE_H = 160;
    QPixmap treeRound = SpriteCache::instance().get(":/sprites/tree_round.png")
                            .scaled(TREE_W, TREE_H, Qt::KeepAspectRatio, Qt::FastTransformation);
    QPixmap treePine  = SpriteCache::instance().get(":/sprites/tree_pine.png")
                            .scaled(TREE_W, TREE_H, Qt::KeepAspectRatio, Qt::FastTransformation);

    struct TreeInfo { QPointF pos; bool round; };
    const QList<TreeInfo> trees = {
        {{40,  40},  false}, {{80,  200}, true }, {{50,  370}, false},
        {{90,  490}, true }, {{680, 55},  true }, {{720, 220}, false},
        {{700, 360}, true }, {{660, 480}, false}, {{155, 130}, true },
        {{575, 150}, false},
    };

    for (const TreeInfo& t : trees) {
        const QPixmap& px = t.round ? treeRound : treePine;
        if (px.isNull()) continue;

        // Shadow ellipse under each tree (same pattern as overworldwidget.cpp)
        const qreal tw = px.width(), th = px.height();
        const qreal sw = tw * 0.65,  sh = th * 0.15;
        auto* shadow = m_scene->addEllipse(
            t.pos.x() + (tw - sw) / 2.0,
            t.pos.y() + th - sh * 0.8,
            sw, sh, Qt::NoPen, Qt::NoBrush);
        QRadialGradient grad(t.pos.x() + tw / 2.0,
                             t.pos.y() + th - sh * 0.3,
                             sw / 2.0);
        grad.setColorAt(0.0, QColor(0, 0, 0, 100));
        grad.setColorAt(0.6, QColor(0, 0, 0, 40));
        grad.setColorAt(1.0, QColor(0, 0, 0, 0));
        shadow->setBrush(grad);
        shadow->setZValue(2);

        auto* treeItem = m_scene->addPixmap(px);
        treeItem->setPos(t.pos);
        treeItem->setZValue(3);
    }

    // ── Dirt wall segments as level-border decoration ─────────────────────────
    QPixmap wallPx = SpriteCache::instance().get(":/sprites/dirt_wall.png");
    if (!wallPx.isNull()) {
        QPixmap wall = wallPx.scaled(TILE, TILE, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        // Top border
        for (int x = 0; x < WORLD_W; x += TILE) {
            auto* w = m_scene->addPixmap(wall);
            w->setPos(x, 0);
            w->setZValue(4);
        }
    }

    // ── Exit portal at the top of the map ─────────────────────────────────────
    const qreal ew = 100, eh = 48;
    const qreal ex = (WORLD_W - ew) / 2.0;
    const qreal ey = 4;

    m_scene->addRect(ex - 10, ey - 5, ew + 20, eh + 10,
                     Qt::NoPen, QBrush(QColor("#1b5e20")))->setZValue(1);

    m_exitZone = m_scene->addRect(ex, ey, ew, eh,
                                  Qt::NoPen, QBrush(QColor("#00e676")));
    m_exitZone->setZValue(2);

    auto* exitLabel = m_scene->addText("EXIT ↑", QFont("Arial", 8, QFont::Bold));
    exitLabel->setDefaultTextColor(Qt::white);
    exitLabel->setPos(ex + 20, ey + 14);
    exitLabel->setZValue(3);

    // ── Level title ───────────────────────────────────────────────────────────
    QString title = m_level.name.isEmpty()
                        ? "LEVEL 1 — FOREST PATH"
                        : m_level.name.toUpper();
    auto* titleText = m_scene->addText(title, QFont("Arial", 8, QFont::Bold));
    titleText->setDefaultTextColor(QColor("#a5d6a7"));
    titleText->setPos(8, 4);
    titleText->setZValue(10);

    // ── Controls hint ─────────────────────────────────────────────────────────
    auto* hint = m_scene->addText(
        "Touch an enemy to fight!    ESC = menu",
        QFont("Arial", 7));
    hint->setDefaultTextColor(QColor("#ffffff80"));
    hint->setPos(8, WORLD_H - 20);
    hint->setZValue(10);

    // ── Player sprite ─────────────────────────────────────────────────────────
    m_player = new DungeonPlayerSprite(m_sheet);
    m_scene->addItem(m_player);
    m_player->setZValue(9);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Player placement and enemy spawning
// ─────────────────────────────────────────────────────────────────────────────

void Level1Widget::placePlayer()
{
    if (m_player)
        m_player->setPos(
            (WORLD_W - DungeonPlayerSprite::W) / 2.0,
            WORLD_H - DungeonPlayerSprite::H - 60
        );
}

void Level1Widget::spawnEnemies()
{
    // Clear previous run's enemies
    for (EnemySprite* e : m_enemies) {
        m_scene->removeItem(e);
        delete e;
    }
    m_enemies.clear();

    // Level 1 has two enemies — easier than the dungeon's three
    struct EInfo { CharacterType type; QString name; QPointF pos; };
    const QList<EInfo> defs = {
        { CharacterType::Warrior, "Forest Guard",    {175, 160} },
        { CharacterType::Archer,  "Woodland Scout",  {575, 230} },
    };

    for (const EInfo& d : defs) {
        auto* e = new EnemySprite(d.type, d.name);
        e->setPos(d.pos);
        e->setZValue(8);
        m_scene->addItem(e);
        m_enemies.append(e);
    }

    // ── Boss — waits at the top of the path ───────────────────────────────────
    m_bossSprite = new EnemySprite(m_level.bossType, m_level.bossName);
    m_bossSprite->setPos(
        (WORLD_W - EnemySprite::W) / 2.0,
        120   // near the top, just below the exit zone
        );
    m_bossSprite->setZValue(8);
    m_scene->addItem(m_bossSprite);
    // Note: boss is NOT added to m_enemies list — handled separately

}

// ─────────────────────────────────────────────────────────────────────────────
//  Game loop
// ─────────────────────────────────────────────────────────────────────────────

void Level1Widget::onTick()
{
    if (m_paused) return;
    movePlayer();
    patrolEnemies();
    checkCollisions();
    fitView();
}

void Level1Widget::movePlayer()
{
    if (!m_player) return;
    m_controller.setSpeed(SPEED);

    QPointF next = m_player->pos() + m_controller.computeVelocity(m_heldKeys);
    next = m_controller.clampToWorld(
        next,
        DungeonPlayerSprite::W, DungeonPlayerSprite::H,
        WORLD_W, WORLD_H
    );
    m_player->setPos(next);
    m_player->updateAnimation(
        m_controller.isMoving(m_heldKeys),
        m_controller.computeDirection(m_heldKeys)
    );
}

void Level1Widget::patrolEnemies()
{
    QRectF bounds(0, 0, WORLD_W, WORLD_H);
    for (EnemySprite* e : m_enemies) {
        e->patrol(bounds);
        e->updateAnimation();
    }
}

void Level1Widget::checkCollisions()
{
    if (!m_player) return;

    // Exit portal
    if (m_exitZone && m_player->collidesWithItem(m_exitZone)) {
        deactivate();
        emit exitedLevel();
        return;
    }

    // ── Boss collision — shows dialog, doesn't start battle directly ──────────
    if (!m_bossTriggered && m_bossSprite
        && m_player->collidesWithItem(m_bossSprite)) {
        m_bossTriggered = true;
        m_ticker.stop();
        emit bossTriggered(m_level);
        return;
    }

    // Enemy contact → trigger a battle (same signal shape as DungeonWidget)
    for (int i = 0; i < m_enemies.size(); ++i) {
        EnemySprite* e = m_enemies[i];
        if (m_player->collidesWithItem(e)) {
            CharacterType type = e->enemyType();
            QString       name = e->enemyName();

            m_scene->removeItem(e);
            delete e;
            m_enemies.removeAt(i);

            deactivate();
            emit battleTriggered(type, name);
            return;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  View / camera
// ─────────────────────────────────────────────────────────────────────────────

void Level1Widget::fitView()
{
    if (!m_view || !m_player) return;
    m_view->resetTransform();
    m_view->scale(2.0, 2.0);          // same zoom as dungeon
    m_view->centerOn(m_player);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Input handling
// ─────────────────────────────────────────────────────────────────────────────

void Level1Widget::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) { togglePause(); return; }
    if (!e->isAutoRepeat()) m_heldKeys.insert(e->key());
    QWidget::keyPressEvent(e);
}

void Level1Widget::keyReleaseEvent(QKeyEvent* e)
{
    if (!e->isAutoRepeat()) m_heldKeys.remove(e->key());
    QWidget::keyReleaseEvent(e);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Resize / pause
// ─────────────────────────────────────────────────────────────────────────────

void Level1Widget::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    fitView();
    if (m_pauseOverlay) m_pauseOverlay->setGeometry(rect());
    if (m_goldHud)      m_goldHud->move(width() - m_goldHud->width() - 16, 16);
}

void Level1Widget::togglePause()
{
    m_paused = !m_paused;
    if (!m_pauseOverlay) return;
    m_pauseOverlay->setGeometry(rect());
    if (m_paused) {
        m_pauseOverlay->show();
        m_pauseOverlay->raise();
    } else {
        m_pauseOverlay->hide();
        setFocus();
    }
}

void Level1Widget::reactivate()
{
    m_bossTriggered = false;   // re-arm in case of reload
    m_heldKeys.clear();
    placePlayer();
    m_ticker.start();
    setFocus();
}
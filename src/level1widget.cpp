#include "level1widget.h"
#include "ui_level1widget.h"
#include "audiomanager.h"
#include "spritecache.h"
#include "pauseoverlaywidget.h"

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

    m_sheet.pixmap = SpriteCache::instance().get(":/sprites/player.png");

    m_view  = ui->graphicsView;
    m_scene = new QGraphicsScene(0, 0, WORLD_W, WORLD_H, this);
    m_view->setScene(m_scene);
    m_view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_view->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing);
    m_view->setFocusPolicy(Qt::NoFocus);

    m_ticker.setInterval(16);
    connect(&m_ticker, &QTimer::timeout, this, &Level1Widget::onTick);

    buildScene();

    setFocusPolicy(Qt::StrongFocus);
    m_view->setSceneRect(0, 0, WORLD_W, WORLD_H);

    m_pauseOverlay = new PauseOverlayWidget(false, this);
    connect(m_pauseOverlay, &PauseOverlayWidget::resumeRequested,
            this, &Level1Widget::togglePause);
    connect(m_pauseOverlay, &PauseOverlayWidget::menuRequested, this, [this] {
        m_paused = false;
        m_pauseOverlay->hide();
        deactivate();
        emit backToMenu();
    });

    m_goldHud = new GoldHudWidget(this);
    m_goldHud->raise();
}

Level1Widget::~Level1Widget() { delete ui; }

// ─────────────────────────────────────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────────────────────────────────────

void Level1Widget::activate(const LevelDef& level, const PlayerProfile& profile)
{
    m_level         = level;
    m_bossTriggered = false;

    if (m_audio) m_audio->playMusic("/music/overworld.ogg");
    m_heldKeys.clear();

    m_scene->clear();
    m_player     = nullptr;
    m_bossSprite = nullptr;
    m_enemyHint  = nullptr;   // ← crash fix
    m_enemies.clear();
    buildScene();

    placePlayer();
    spawnEnemies();
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

void Level1Widget::reactivate()
{
    m_bossTriggered = false;
    m_heldKeys.clear();
    placePlayer();
    m_ticker.start();
    setFocus();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Scene construction
// ─────────────────────────────────────────────────────────────────────────────

void Level1Widget::buildScene()
{
    const int TILE = 64;

    // ── Grass base ────────────────────────────────────────────────────────────
    QPixmap grassTile = SpriteCache::instance().get(":/sprites/grass.png")
                            .scaled(TILE, TILE, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    for (int y = 0; y < WORLD_H; y += TILE)
        for (int x = 0; x < WORLD_W; x += TILE) {
            auto* t = m_scene->addPixmap(grassTile);
            t->setPos(x, y); t->setZValue(0);
        }

    // ── Dirt path ─────────────────────────────────────────────────────────────
    QPixmap d1 = SpriteCache::instance().get(":/sprites/dirt1.png")
                     .scaled(TILE, TILE, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    QPixmap d2 = SpriteCache::instance().get(":/sprites/dirt2.png")
                     .scaled(TILE, TILE, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    auto* rng = QRandomGenerator::global();
    const int pathX = 340, pathW = 128;
    for (int y = 0; y < WORLD_H; y += TILE)
        for (int x = pathX; x < pathX + pathW; x += TILE) {
            auto* t = m_scene->addPixmap(rng->bounded(100) < 24 ? d2 : d1);
            t->setPos(x, y); t->setZValue(1);
        }

    // ── Trees ─────────────────────────────────────────────────────────────────
    const int TREE_W = 160, TREE_H = 160;
    QPixmap treeRound = SpriteCache::instance().get(":/sprites/tree_round.png")
                            .scaled(TREE_W, TREE_H, Qt::KeepAspectRatio, Qt::FastTransformation);
    QPixmap treePine  = SpriteCache::instance().get(":/sprites/tree_pine.png")
                            .scaled(TREE_W, TREE_H, Qt::KeepAspectRatio, Qt::FastTransformation);

    struct TreeInfo { QPointF pos; bool round; };
    const QList<TreeInfo> trees = {
        {{40,40},false},{{80,200},true},{{50,370},false},{{90,490},true},
        {{680,55},true},{{720,220},false},{{700,360},true},{{660,480},false},
        {{155,130},true},{{575,150},false},
    };
    for (const TreeInfo& t : trees) {
        const QPixmap& px = t.round ? treeRound : treePine;
        if (px.isNull()) continue;
        const qreal tw = px.width(), th = px.height();
        const qreal sw = tw*0.65, sh = th*0.15;
        auto* shadow = m_scene->addEllipse(
            t.pos.x()+(tw-sw)/2.0, t.pos.y()+th-sh*0.8, sw, sh, Qt::NoPen, Qt::NoBrush);
        QRadialGradient grad(t.pos.x()+tw/2.0, t.pos.y()+th-sh*0.3, sw/2.0);
        grad.setColorAt(0.0, QColor(0,0,0,100));
        grad.setColorAt(0.6, QColor(0,0,0,40));
        grad.setColorAt(1.0, QColor(0,0,0,0));
        shadow->setBrush(grad); shadow->setZValue(2);
        auto* ti = m_scene->addPixmap(px);
        ti->setPos(t.pos); ti->setZValue(3);
    }

    // ── Dirt wall top border ──────────────────────────────────────────────────
    QPixmap wallPx = SpriteCache::instance().get(":/sprites/dirt_wall.png");
    if (!wallPx.isNull()) {
        QPixmap wall = wallPx.scaled(TILE, TILE, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        for (int x = 0; x < WORLD_W; x += TILE) {
            auto* w = m_scene->addPixmap(wall);
            w->setPos(x, 0); w->setZValue(4);
        }
    }

    // ── Exit portal ───────────────────────────────────────────────────────────
    const qreal ew = 100, eh = 48;
    const qreal ex = (WORLD_W - ew) / 2.0, ey = 4;

    // Exit color varies by theme
    QColor exitColor;
    switch (m_level.theme) {
    case LevelTheme::Forest:  exitColor = QColor("#7B1FA2"); break;  // purple
    case LevelTheme::Peak:    exitColor = QColor("#0288D1"); break;  // ice blue
    case LevelTheme::Volcano: exitColor = QColor("#E64A19"); break;  // lava orange
    case LevelTheme::Arena:   exitColor = QColor("#F9A825"); break;  // gold
    default:                  exitColor = QColor("#00e676"); break;  // green (cave)
    }

    m_scene->addRect(ex-10, ey-5, ew+20, eh+10, Qt::NoPen,
                     QBrush(exitColor.darker(200)))->setZValue(1);
    m_exitZone = m_scene->addRect(ex, ey, ew, eh, Qt::NoPen, QBrush(exitColor));
    m_exitZone->setZValue(2);

    auto* exitLabel = m_scene->addText("EXIT ↑", QFont("Arial", 8, QFont::Bold));
    exitLabel->setDefaultTextColor(Qt::white);
    exitLabel->setPos(ex+20, ey+14); exitLabel->setZValue(3);

    // ── Level title ───────────────────────────────────────────────────────────
    QString title = m_level.name.isEmpty() ? "LEVEL 1" : m_level.name.toUpper();
    auto* titleText = m_scene->addText(title, QFont("Arial", 8, QFont::Bold));
    titleText->setDefaultTextColor(QColor("#a5d6a7"));
    titleText->setPos(8, 4); titleText->setZValue(10);

    // ── Controls hint ─────────────────────────────────────────────────────────
    auto* hint = m_scene->addText("Touch an enemy to fight!    ESC = menu",
                                   QFont("Arial", 7));
    hint->setDefaultTextColor(QColor("#ffffff80"));
    hint->setPos(8, WORLD_H - 20); hint->setZValue(10);

    // ── Enemy hint ────────────────────────────────────────────────────────────
    m_enemyHint = m_scene->addText("", QFont("Arial", 8, QFont::Bold));
    m_enemyHint->setDefaultTextColor(QColor("#FFE066"));
    m_enemyHint->setPos(8, WORLD_H - 40); m_enemyHint->setZValue(10);

    // ── Player sprite ─────────────────────────────────────────────────────────
    m_player = new DungeonPlayerSprite(m_sheet);
    m_scene->addItem(m_player);
    m_player->setZValue(9);

    // ── Theme-specific elements ───────────────────────────────────────────────
    buildThemeElements();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Theme elements — drawn on top of the base scene
// ─────────────────────────────────────────────────────────────────────────────

void Level1Widget::buildThemeElements()
{
    auto* rng = QRandomGenerator::global();

    switch (m_level.theme) {

    // ── Cave: dark rock debris + stalactites ──────────────────────────────────
    case LevelTheme::Cave: {
        // Scattered dark rock rects
        const QList<QPointF> rocks = {
            {120,180},{200,320},{560,250},{640,400},{300,450},{480,130}
        };
        for (const QPointF& p : rocks) {
            qreal w = 18 + rng->bounded(20);
            qreal h = 10 + rng->bounded(12);
            m_scene->addRect(p.x(), p.y(), w, h,
                Qt::NoPen, QBrush(QColor("#37474f")))->setZValue(5);
        }
        // Stalactites hanging from top wall — simple triangles using thin rects
        for (int x = 40; x < WORLD_W - 40; x += 80 + rng->bounded(40)) {
            int h = 20 + rng->bounded(30);
            // Draw as a tapered stack of rects
            for (int i = 0; i < 3; i++) {
                int w = 14 - i*4;
                m_scene->addRect(x + i*2, 64 + i*8, w, h/3,
                    Qt::NoPen, QBrush(QColor(55,71,79,200)))->setZValue(5);
            }
        }
        break;
    }

    // ── Forest: dark overlay + skulls + dead trunks ───────────────────────────
    case LevelTheme::Forest: {
        // Full-screen dark purple overlay
        auto* overlay = m_scene->addRect(0, 0, WORLD_W, WORLD_H,
            Qt::NoPen, QBrush(QColor(30, 0, 50, 140)));
        overlay->setZValue(6);

        // Floating skull symbols scattered around
        const QList<QPointF> skulls = {
            {95,95},{310,160},{580,90},{150,380},{670,310},{420,470},{240,520}
        };
        for (const QPointF& p : skulls) {
            auto* skull = m_scene->addText("☠", QFont("Arial", 10));
            skull->setDefaultTextColor(QColor(180, 100, 200, 160));
            skull->setPos(p); skull->setZValue(7);
        }

        // Dead tree trunks — thin dark rects with no foliage
        const QList<QPointF> trunks = {
            {110,200},{580,300},{200,430},{660,150}
        };
        for (const QPointF& p : trunks) {
            int h = 80 + rng->bounded(60);
            // Trunk
            m_scene->addRect(p.x(), p.y(), 8, h,
                Qt::NoPen, QBrush(QColor("#263238")))->setZValue(5);
            // Two bare branches
            m_scene->addRect(p.x()-20, p.y()+20, 20, 5,
                Qt::NoPen, QBrush(QColor("#263238")))->setZValue(5);
            m_scene->addRect(p.x()+8, p.y()+40, 20, 5,
                Qt::NoPen, QBrush(QColor("#263238")))->setZValue(5);
        }
        break;
    }

    // ── Peak: blue overlay + snowflakes + ice shards ──────────────────────────
    case LevelTheme::Peak: {
        // Blue/white icy overlay
        auto* overlay = m_scene->addRect(0, 0, WORLD_W, WORLD_H,
            Qt::NoPen, QBrush(QColor(0, 80, 160, 100)));
        overlay->setZValue(6);

        // Snowflake dots scattered
        for (int i = 0; i < 30; i++) {
            int x = rng->bounded(WORLD_W);
            int y = rng->bounded(WORLD_H);
            int r = 2 + rng->bounded(4);
            m_scene->addEllipse(x, y, r, r,
                Qt::NoPen, QBrush(QColor(200, 230, 255, 180)))->setZValue(7);
        }

        // Angular ice shard rects at ground edges
        const QList<QPointF> shards = {
            {60,500},{160,520},{600,490},{700,510},{280,540},{450,530}
        };
        for (const QPointF& p : shards) {
            int h = 30 + rng->bounded(40);
            m_scene->addRect(p.x(), p.y()-h, 12, h,
                Qt::NoPen, QBrush(QColor(150, 210, 255, 200)))->setZValue(5);
            m_scene->addRect(p.x()+6, p.y()-h*0.7, 8, h*0.7,
                Qt::NoPen, QBrush(QColor(180, 230, 255, 180)))->setZValue(5);
        }
        break;
    }

    // ── Volcano: red overlay + lava glow pools + dark rock rects ─────────────
    case LevelTheme::Volcano: {
        // Red/orange fiery overlay
        auto* overlay = m_scene->addRect(0, 0, WORLD_W, WORLD_H,
            Qt::NoPen, QBrush(QColor(180, 30, 0, 110)));
        overlay->setZValue(6);

        // Lava pools — glowing orange ellipses at ground level
        const QList<QPointF> pools = {
            {80,480},{220,510},{550,490},{680,520},{350,540}
        };
        for (const QPointF& p : pools) {
            int w = 40 + rng->bounded(30), h = 16 + rng->bounded(10);
            // Outer glow
            m_scene->addEllipse(p.x()-4, p.y()-4, w+8, h+8,
                Qt::NoPen, QBrush(QColor(255, 100, 0, 60)))->setZValue(5);
            // Lava surface
            m_scene->addEllipse(p.x(), p.y(), w, h,
                Qt::NoPen, QBrush(QColor(255, 140, 0, 180)))->setZValue(5);
        }

        // Dark volcanic rock rects
        const QList<QPointF> rocks = {
            {130,300},{290,200},{570,350},{680,250},{400,480}
        };
        for (const QPointF& p : rocks) {
            int w = 24+rng->bounded(20), h = 14+rng->bounded(14);
            m_scene->addRect(p.x(), p.y(), w, h,
                Qt::NoPen, QBrush(QColor("#1a0000")))->setZValue(5);
        }
        break;
    }

    // ── Arena: dark overlay + stone boundary ring + corner torches ────────────
    case LevelTheme::Arena: {
        // Dark overlay
        auto* overlay = m_scene->addRect(0, 0, WORLD_W, WORLD_H,
            Qt::NoPen, QBrush(QColor(0, 0, 0, 130)));
        overlay->setZValue(6);

        // Arena boundary — four thick border lines drawn as rects
        const int margin = 50, thick = 8;
        QColor borderCol("#B8860B");
        m_scene->addRect(margin, margin, WORLD_W-margin*2, thick,
            Qt::NoPen, QBrush(borderCol))->setZValue(7);  // top
        m_scene->addRect(margin, WORLD_H-margin-thick, WORLD_W-margin*2, thick,
            Qt::NoPen, QBrush(borderCol))->setZValue(7);  // bottom
        m_scene->addRect(margin, margin, thick, WORLD_H-margin*2,
            Qt::NoPen, QBrush(borderCol))->setZValue(7);  // left
        m_scene->addRect(WORLD_W-margin-thick, margin, thick, WORLD_H-margin*2,
            Qt::NoPen, QBrush(borderCol))->setZValue(7);  // right

        // Corner torches — glow + flame dot
        const QList<QPointF> corners = {
            {margin-8, margin-8}, {WORLD_W-margin-8, margin-8},
            {margin-8, WORLD_H-margin-8}, {WORLD_W-margin-8, WORLD_H-margin-8}
        };
        for (const QPointF& p : corners) {
            m_scene->addEllipse(p.x()-6, p.y()-6, 20, 20,
                Qt::NoPen, QBrush(QColor(255,160,0,80)))->setZValue(7);
            m_scene->addEllipse(p.x(), p.y(), 8, 8,
                Qt::NoPen, QBrush(QColor("#FFD700")))->setZValue(8);
        }
        break;
    }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Player and enemies
// ─────────────────────────────────────────────────────────────────────────────

void Level1Widget::placePlayer()
{
    if (m_player)
        m_player->setPos(
            (WORLD_W - DungeonPlayerSprite::W) / 2.0,
            WORLD_H - DungeonPlayerSprite::H - 60);
}

void Level1Widget::spawnEnemies()
{
    for (EnemySprite* e : m_enemies) { m_scene->removeItem(e); delete e; }
    m_enemies.clear();

    // Enemy name and type driven by the level's theme
    struct EInfo { CharacterType type; QString name; QPointF pos; };

    QList<EInfo> defs;
    switch (m_level.theme) {
    case LevelTheme::Cave:
        defs = {{ CharacterType::Warrior, "Cave Goblin", {300, 300} }};
        break;
    case LevelTheme::Forest:
        defs = {{ CharacterType::Archer,  "Dead Archer",  {300, 300} }};
        break;
    case LevelTheme::Peak:
        defs = {{ CharacterType::Warrior, "Ice Knight",   {300, 300} }};
        break;
    case LevelTheme::Volcano:
        defs = {{ CharacterType::Mage,    "Fire Wizard",  {300, 300} }};
        break;
    case LevelTheme::Arena:
        defs = {{ CharacterType::Warrior, "Dark Knight",  {300, 300} }};
        break;
    }

    for (const EInfo& d : defs) {
        auto* e = new EnemySprite(d.type, d.name);
        e->setPos(d.pos); e->setZValue(8);
        m_scene->addItem(e);
        m_enemies.append(e);
    }

    // Boss
    m_bossSprite = new EnemySprite(m_level.bossType, m_level.bossName);
    m_bossSprite->setPos((WORLD_W - EnemySprite::W) / 2.0, 120);
    m_bossSprite->setZValue(8);
    m_scene->addItem(m_bossSprite);
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

    if (m_enemyHint) {
        if (!m_enemies.isEmpty())
            m_enemyHint->setPlainText(
                QString("⚠ Defeat all enemies first! (%1 remaining)").arg(m_enemies.size()));
        else if (!m_bossTriggered)
            m_enemyHint->setPlainText("✦ The boss awaits — move north!");
        else
            m_enemyHint->setPlainText("");
    }
}

void Level1Widget::movePlayer()
{
    if (!m_player) return;
    m_controller.setSpeed(SPEED);
    QPointF next = m_player->pos() + m_controller.computeVelocity(m_heldKeys);
    next = m_controller.clampToWorld(next,
        DungeonPlayerSprite::W, DungeonPlayerSprite::H, WORLD_W, WORLD_H);
    m_player->setPos(next);
    m_player->updateAnimation(
        m_controller.isMoving(m_heldKeys),
        m_controller.computeDirection(m_heldKeys));
}

void Level1Widget::patrolEnemies()
{
    QRectF bounds(0, 0, WORLD_W, WORLD_H);
    for (EnemySprite* e : m_enemies) { e->patrol(bounds); e->updateAnimation(); }
}

void Level1Widget::checkCollisions()
{
    if (!m_player) return;

    // Boss — checked BEFORE exit, requires enemies cleared
    if (!m_bossTriggered && m_bossSprite
        && m_enemies.isEmpty()
        && m_player->collidesWithItem(m_bossSprite)) {
        m_bossTriggered = true;
        m_ticker.stop();
        emit bossTriggered(m_level);
        return;
    }

    // Exit portal
    if (m_exitZone && m_player->collidesWithItem(m_exitZone)) {
        deactivate();
        emit exitedLevel();
        return;
    }

    // Regular enemies
    for (int i = 0; i < m_enemies.size(); ++i) {
        EnemySprite* e = m_enemies[i];
        if (m_player->collidesWithItem(e)) {
            CharacterType type = e->enemyType();
            QString       name = e->enemyName();
            m_scene->removeItem(e); delete e; m_enemies.removeAt(i);
            deactivate();
            emit battleTriggered(type, name);
            return;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  View / input / resize / pause
// ─────────────────────────────────────────────────────────────────────────────

void Level1Widget::fitView()
{
    if (!m_view || !m_player) return;
    m_view->resetTransform();
    m_view->scale(2.0, 2.0);
    m_view->centerOn(m_player);
}

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
    if (m_paused) { m_pauseOverlay->show(); m_pauseOverlay->raise(); }
    else          { m_pauseOverlay->hide(); setFocus(); }
}

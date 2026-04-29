#include "dungeonwidget.h"
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QRandomGenerator>
#include <QGraphicsEllipseItem>
#include <QRadialGradient>
#include "audiomanager.h"
#include "spritecache.h"
#include "pauseoverlaywidget.h"
#include "overworldwidget.h"  // to reuse Direction enum

// ═════════════════════════════════════════════════════════════════════════════
//  DungeonPlayerSprite — same pattern as OverworldWidget's PlayerSprite
// ═════════════════════════════════════════════════════════════════════════════

DungeonPlayerSprite::DungeonPlayerSprite(const DungeonSpriteSheet &sheet, 
                                         QGraphicsItem *parent)
    : QGraphicsPixmapItem(parent)
    , m_sheet(sheet)
{
    setTransformationMode(Qt::FastTransformation);
    setTransformOriginPoint(W / 2.0, H / 2.0);
    
    setIdleFrame(Direction::Down);
    
    // Shadow under player
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
    m_isIdle     = true;
    m_facing     = dir;
    m_frameIndex = 0;
    m_tickAccum  = 0;
    applyFrame();
}

void DungeonPlayerSprite::setWalkAnim(Direction dir)
{
    if (!m_isIdle && m_facing == dir) return;
    
    m_isIdle     = false;
    m_facing     = dir;
    m_frameIndex = 0;
    m_tickAccum  = 0;
    applyFrame();
}

void DungeonPlayerSprite::applyFrame()
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

void DungeonPlayerSprite::updateAnimation(bool isMoving, Direction facingDir)
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

// ═════════════════════════════════════════════════════════════════════════════
//  EnemySprite — now with proper sprites instead of colored rectangles
// ═════════════════════════════════════════════════════════════════════════════

static QPixmap getEnemySprite(CharacterType t)
{
    QString path;
    switch (t) {
    case CharacterType::Warrior: path = ":/sprites/warrior_front.png"; break;
    case CharacterType::Mage:    path = ":/sprites/mage_front.png";    break;
    case CharacterType::Archer:  path = ":/sprites/archer_front.png";  break;
    }
    return SpriteCache::instance().getScaled(path, 
                                             static_cast<int>(EnemySprite::W), 
                                             static_cast<int>(EnemySprite::H));
}

EnemySprite::EnemySprite(CharacterType type, const QString &name, 
                         QGraphicsItem *parent)
    : QGraphicsPixmapItem(parent)
    , m_type(type)
    , m_name(name)
{
    m_sprite = getEnemySprite(type);
    setPixmap(m_sprite);
    setTransformationMode(Qt::FastTransformation);
    
    // Random starting velocity
    auto *rng = QRandomGenerator::global();
    m_vx = (rng->bounded(2) == 0 ? 1 : -1) * (1 + rng->bounded(2));
    m_vy = (rng->bounded(2) == 0 ? 1 : -1) * (1 + rng->bounded(2));
    
    // Shadow under enemy
    const qreal shadowW = W * 0.60;
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
    
    // Name label above enemy
    m_nameLabel = new QGraphicsTextItem(name, this);
    m_nameLabel->setDefaultTextColor(QColor("#ffcccc"));
    m_nameLabel->setFont(QFont("Arial", 6));
    
    // Center the label
    qreal labelW = m_nameLabel->boundingRect().width();
    m_nameLabel->setPos((W - labelW) / 2.0, -14);
    m_nameLabel->setZValue(1);
}

void EnemySprite::patrol(const QRectF &worldBounds)
{
    qreal nx = x() + m_vx;
    qreal ny = y() + m_vy;
    
    // Bounce off edges
    if (nx < worldBounds.left()       || nx > worldBounds.right()  - W) { 
        m_vx = -m_vx; 
        nx = x(); 
    }
    if (ny < worldBounds.top() + 60   || ny > worldBounds.bottom() - H) { 
        m_vy = -m_vy; 
        ny = y(); 
    }
    
    setPos(nx, ny);
}

void EnemySprite::updateAnimation()
{
    // Simple idle animation: subtle bob or frame cycle
    ++m_tickAccum;
    if (m_tickAccum >= 30) {  // slower than player walk
        m_tickAccum = 0;
        m_frameIndex = (m_frameIndex + 1) % 2;  // simple 2-frame idle
        
        // Optional: add slight vertical bob
        qreal bob = (m_frameIndex == 0) ? 0.0 : 1.0;
        setY(y() - bob);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  DungeonWidget
// ═════════════════════════════════════════════════════════════════════════════

DungeonWidget::DungeonWidget(AudioManager *audio, QWidget *parent)
    : QWidget(parent)
    , m_audio(audio)
{
    // Load player sprite sheet (reuse same one as overworld)
    m_sheet.pixmap = SpriteCache::instance().get(":/sprites/player.png");
    
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_scene = new QGraphicsScene(0, 0, WORLD_W, WORLD_H, this);

    m_scene = new QGraphicsScene(0, 0, WORLD_W, WORLD_H, this);

    m_combat.setScene(m_scene);
  
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

    m_ticker.setInterval(16);  // ~60 fps
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
}

void DungeonWidget::activate()
{
    if (m_audio) m_audio->playMusic("/music/battle.ogg");
    m_heldKeys.clear();
    placePlayer();
    spawnEnemies();
    m_ticker.start();
    setFocus();
}

void DungeonWidget::deactivate()
{
    m_ticker.stop();
    m_heldKeys.clear();
}

void DungeonWidget::buildScene() {
    // Dark stone floor
    m_scene->setBackgroundBrush(QColor("#1a1a2e"));

    // Stone tile grid
    const int tileW = 80, tileH = 60;
    for (int col = 0; col < WORLD_W / tileW; ++col) {
        for (int row = 0; row < WORLD_H / tileH; ++row) {
            m_scene->addRect(col * tileW + 1, row * tileH + 1,
                             tileW - 2, tileH - 2,
                             QPen(QColor("#263238"), 1),
                             QBrush(QColor("#263238")))->setZValue(0);
        }
    }

    // Wall pillars
    const QList<QPointF> pillars = {
        {160, 120}, {400, 120}, {620, 120},
        {160, 340}, {400, 340}, {620, 340},
        {80,  460}, {500, 460},
    };
    for (const QPointF &p : pillars) {
        m_scene->addRect(p.x(), p.y(), 32, 48,
                         QPen(QColor("#37474f"), 2),
                         QBrush(QColor("#455a64")))->setZValue(1);
    }

    // Torches (glowing yellow dots)
    const QList<QPointF> torches = {
        {100, 80}, {440, 80}, {720, 80},
        {100, 480},{440, 480},{720, 480},
    };
    for (const QPointF &t : torches) {
        auto *glow = m_scene->addEllipse(t.x() - 8, t.y() - 8, 20, 20,
                                         Qt::NoPen, QBrush(QColor("#ff6f0080")));
        glow->setZValue(1);
        m_scene->addEllipse(t.x() - 2, t.y() - 2, 8, 8,
                            Qt::NoPen, QBrush(QColor("#ffca28")))->setZValue(2);
    }

    // Exit portal (bottom-center)
    const qreal ew = 80, eh = 56;
    const qreal ex = (WORLD_W - ew) / 2.0;
    const qreal ey = WORLD_H - eh - 6;

    m_scene->addRect(ex - 10, ey - 6, ew + 20, eh + 10,
                     Qt::NoPen, QBrush(QColor("#1b5e20")))->setZValue(1);

    m_exitZone = m_scene->addRect(ex, ey, ew, eh,
                                  Qt::NoPen, QBrush(QColor("#00c853")));
    m_exitZone->setZValue(2);

    auto *exitLabel = m_scene->addText("EXIT ↑", QFont("Arial", 8, QFont::Bold));
    exitLabel->setDefaultTextColor(Qt::white);
    exitLabel->setPos(ex + 12, ey + 18);
    exitLabel->setZValue(3);

    // HUD hint
    auto *hint = m_scene->addText(
        "Touch an enemy to fight!    ESC = menu",
        QFont("Arial", 7));
    hint->setDefaultTextColor(QColor("#ffffff88"));
    hint->setPos(8, 4);
    hint->setZValue(10);

    // Player sprite
    m_player = new DungeonPlayerSprite(m_sheet);
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

    m_player->setPos((WORLD_W - DungeonPlayerSprite::W) / 2.0,
                     WORLD_H - DungeonPlayerSprite::H - 90);
}

void DungeonWidget::spawnEnemies()
{
    for (EnemySprite* enemy : m_enemies) {
        m_scene->removeItem(enemy);
        delete enemy;
    }
    m_enemies.clear();

    struct EnemyInfo {
        CharacterType type;
        QString name;
        QPointF pos;
    };

    const QList<EnemyInfo> enemies = {
        { CharacterType::Warrior, "Guard",  QPointF(120, 130) },
        { CharacterType::Mage,    "Mage",   QPointF(380, 210) },
        { CharacterType::Archer,  "Archer", QPointF(620, 150) }
    };

    for (const EnemyInfo& info : enemies) {
        EnemySprite* enemy = new EnemySprite(info.type, info.name);
        enemy->setPos(info.pos);
        enemy->setZValue(8);
        m_scene->addItem(enemy);
        m_enemies.append(enemy);
    }
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
}

void DungeonWidget::onTick()
{
    if (m_paused)
        return;

    movePlayer();
    patrolEnemies();
    checkCollisions();
    fitView();
}

void DungeonWidget::movePlayer()
{
    if (!m_player)
        return;

    m_controller.setSpeed(SPEED);

    QPointF velocity = m_controller.computeVelocity(m_heldKeys);
    QPointF nextPos = m_player->pos() + velocity;

    nextPos = m_controller.clampToWorld(
        nextPos,
        DungeonPlayerSprite::W,
        DungeonPlayerSprite::H,
        WORLD_W,
        WORLD_H
    );

    m_player->setPos(nextPos);

    bool moving = m_controller.isMoving(m_heldKeys);
    Direction dir = m_controller.computeDirection(m_heldKeys);
    m_player->updateAnimation(moving, dir);
    if (m_controller.isMoving(m_heldKeys))
    m_facing = dir;
  
}

void DungeonWidget::patrolEnemies()
{
    QRectF bounds(0, 0, WORLD_W, WORLD_H);

    for (EnemySprite* enemy : m_enemies) {
        enemy->patrol(bounds);
        enemy->updateAnimation();
    }
}

void DungeonWidget::checkCollisions()
{
    if (!m_player)
        return;

    if (m_exitZone && m_player->collidesWithItem(m_exitZone)) {
        deactivate();
        emit exitedDungeon();
        return;
    }

}

void DungeonWidget::fitView()
{
    if (!m_view || !m_player)
        return;

    m_view->resetTransform();

    // Zoom in
    m_view->scale(2.0, 2.0);

    // Camera follows player
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

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

    setIdleFrame(Direction::Down);

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
    m_isIdle = true;
    m_facing = dir;
    m_frameIndex = 0;
    m_tickAccum = 0;
    applyFrame();
}

void PlayerSprite::setWalkAnim(Direction dir)
{
    if (!m_isIdle && m_facing == dir) return;

    m_isIdle = false;
    m_facing = dir;
    m_frameIndex = 0;
    m_tickAccum = 0;
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

void PlayerSprite::step(const QSet<int> &heldKeys, const QRectF &worldBounds)
{
    const bool goUp    = heldKeys.contains(Qt::Key_W) || heldKeys.contains(Qt::Key_Up);
    const bool goDown  = heldKeys.contains(Qt::Key_S) || heldKeys.contains(Qt::Key_Down);
    const bool goLeft  = heldKeys.contains(Qt::Key_A) || heldKeys.contains(Qt::Key_Left);
    const bool goRight = heldKeys.contains(Qt::Key_D) || heldKeys.contains(Qt::Key_Right);

    qreal dx = 0, dy = 0;

    if (goUp) dy -= SPEED;
    if (goDown) dy += SPEED;
    if (goLeft) dx -= SPEED;
    if (goRight) dx += SPEED;

    if (dx != 0 && dy != 0) { dx *= 0.7071; dy *= 0.7071; }

    const bool moving = (dx != 0 || dy != 0);

    if (moving) {
        Direction dir;

        if      (goUp && goRight) dir = Direction::ForwardRight;
        else if (goUp && goLeft)  dir = Direction::ForwardLeft;
        else if (goDown && goRight) dir = Direction::DownRight;
        else if (goDown && goLeft)  dir = Direction::DownLeft;
        else if (goRight) dir = Direction::Right;
        else if (goLeft)  dir = Direction::Left;
        else if (goUp)    dir = Direction::Up;
        else              dir = Direction::Down;

        setWalkAnim(dir);

        ++m_tickAccum;

        if (m_tickAccum >= TICKS_PER_FRAME) {
            m_tickAccum = 0;
            m_frameIndex = (m_frameIndex + 1) % WALK_FRAMES;
            applyFrame();
        }

    } else {
        if (!m_isIdle) setIdleFrame(m_facing);
    }

    qreal nx = qBound(worldBounds.left(), x() + dx, worldBounds.right() - W);
    qreal ny = qBound(worldBounds.top(),  y() + dy, worldBounds.bottom() - H);

    setPos(nx, ny);
}


// ═════════════════════════════════════════════════════════════════════════════
//  OverworldWidget
// ═════════════════════════════════════════════════════════════════════════════

OverworldWidget::OverworldWidget(AudioManager *audio, QWidget *parent)
    : QWidget(parent), m_audio(audio)
{
    QString spritePath = ":/sprites/player.png";

    m_sheet.pixmap = QPixmap(spritePath);

    if (m_sheet.pixmap.isNull())
        qFatal("Could not load player sprite");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);

    m_scene = new QGraphicsScene(0,0,WORLD_W,WORLD_H,this);

    m_view = new QGraphicsView(m_scene,this);

    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // FIX grid seams
    m_view->setRenderHint(QPainter::SmoothPixmapTransform,false);
    m_view->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing);
    m_view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    m_view->setFocusPolicy(Qt::NoFocus);

    layout->addWidget(m_view);

    m_ticker.setInterval(16);

    connect(&m_ticker,&QTimer::timeout,this,&OverworldWidget::onTick);

    buildScene();

    setFocusPolicy(Qt::StrongFocus);

    m_view->setSceneRect(0,0,WORLD_W,WORLD_H);

    buildPauseOverlay();
}

void OverworldWidget::buildScene()
{
    QPixmap grassTile(":/sprites/grass.png");

    const int TILE_SIZE = 64;

    QPixmap scaledTile = grassTile.scaled(
        TILE_SIZE+1, TILE_SIZE+1,
        Qt::IgnoreAspectRatio,
        Qt::FastTransformation);

    for (int y=0; y<WORLD_H; y+=TILE_SIZE)
    {
        for (int x=0; x<WORLD_W; x+=TILE_SIZE)
        {
            QGraphicsPixmapItem *tile = m_scene->addPixmap(scaledTile);
            tile->setPos(x-0.5,y-0.5);
            tile->setZValue(0);
        }
    }

    QPixmap treeRoundPx("resources/sprites/tree_round.png");
    QPixmap treePinePx ("resources/sprites/tree_pine.png");

    const int TREE_W = 160;
    const int TREE_H = 160;

    QPixmap treeRound = treeRoundPx.scaled(TREE_W,TREE_H,Qt::KeepAspectRatio,Qt::FastTransformation);
    QPixmap treePine  = treePinePx.scaled (TREE_W,TREE_H,Qt::KeepAspectRatio,Qt::FastTransformation);

    const QList<QPair<QPointF,bool>> trees = {
        {{60,60},false},
        {{150,50},true},
        {{680,55},true},
        {{740,130},false}
    };

    for (const auto &[p,isRound] : trees)
    {
        const QPixmap &px = isRound ? treeRound : treePine;

        const qreal tw = px.width();
        const qreal th = px.height();

        const qreal shadowW = tw * 0.70;
        const qreal shadowH = th * 0.25;

        const qreal shadowX = p.x() + (tw-shadowW)/2.0;
        const qreal shadowY = p.y() + th - shadowH * 0.8;

        auto *shadow = m_scene->addEllipse(shadowX,shadowY,shadowW,shadowH,Qt::NoPen,Qt::NoBrush);

        QRadialGradient grad(shadowW/2.0,shadowH/2.0,shadowW/2.0);
        grad.setColorAt(0.0,QColor(0,0,0,140));
        grad.setColorAt(0.7,QColor(0,0,0,60));
        grad.setColorAt(1.0,QColor(0,0,0,0));

        shadow->setBrush(grad);
        shadow->setZValue(2);

        auto *treeItem = m_scene->addPixmap(px);
        treeItem->setPos(p.x(),p.y());
        treeItem->setZValue(3);
    }

    m_player = new PlayerSprite(m_sheet);
    m_player->setZValue(9);

    m_scene->addItem(m_player);

    placePlayer();
}

void OverworldWidget::fitView()
{
    if(!m_view) return;

    m_view->fitInView(0,0,WORLD_W,WORLD_H,Qt::KeepAspectRatio);
}

#include "level1widget.h"
#include "audiomanager.h"
#include "pauseoverlaywidget.h"
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QRandomGenerator>
#include <QtMath>
#include <QLabel>
#include <QHBoxLayout>
#include "mage.h"
#include "archer.h"
#include "warrior.h"
// ── Static sprite sheet paths ─────────────────────────────────────────────────
// Change these two lines when you have unique sprites per level
const QString Level1Widget::ENEMY_MOVEMENT_SHEET =
    ":/sprites/goblin_brute_movement_8dir_6frames.png";
const QString Level1Widget::ENEMY_ATTACK_SHEET =
    ":/sprites/goblin_brute_attack_8dir_7frames.png";

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────

Level1Widget::Level1Widget(AudioManager* audio, QWidget* parent)
    : QWidget(parent), m_audio(audio)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_scene = new QGraphicsScene(0, 0, WORLD_W, WORLD_H, this);

    m_view = new QGraphicsView(m_scene, this);
    m_view->setFrameShape(QFrame::NoFrame);
    m_view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(m_view);

    m_goldHud = new GoldHudWidget(this);
    m_goldHud->raise();

    m_pauseOverlay = new PauseOverlayWidget(false, this);
    m_pauseOverlay->hide();

    connect(m_pauseOverlay, &PauseOverlayWidget::resumeRequested,
            this, &Level1Widget::togglePause);
    connect(m_pauseOverlay, &PauseOverlayWidget::menuRequested,
            this, [this]{
                deactivate();
                emit backToMenu();
            });

    m_ticker.setInterval(16);
    connect(&m_ticker, &QTimer::timeout, this, &Level1Widget::onTick);

    setFocusPolicy(Qt::StrongFocus);
    buildPlayerHud();
}

Level1Widget::~Level1Widget()
{
    clearEnemies();
    delete m_bossLogic;
    delete m_levelPlayer;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────────────────────────────────────

void Level1Widget::setPlayerCharacterType(CharacterType type)
{
    m_playerType = type;
    loadPlayerSheet(type);
    loadAttackSheet(type);
}

void Level1Widget::activate(const LevelDef& level, const PlayerProfile& profile)
{
    m_level          = level;
    m_bossActive     = false;
    m_bossTriggered  = false;
    m_paused         = false;
    m_heldKeys.clear();
    m_facing = Direction::Down;

    // Rebuild scene fresh each time
    m_scene->clear();
    m_player     = nullptr;
    m_bossSprite = nullptr;
    clearEnemies();

    delete m_bossLogic;
    m_bossLogic = nullptr;
    m_exitArea  = QRectF();

    // Reset combat manager
    m_combat.clearEnemies();
    m_combat.clearAttacks();
    m_combat.setScene(m_scene);

    // Create a fresh character for this level run.
    // Owned by Level1Widget, passed to WorldCombatManager as a raw pointer.
    delete m_levelPlayer;
    switch (m_playerType) {
    case CharacterType::Mage:   m_levelPlayer = new Mage  ("Player"); break;
    case CharacterType::Archer: m_levelPlayer = new Archer("Player"); break;
    default:                    m_levelPlayer = new Warrior("Player"); break;
    }
    // Apply permanent stat upgrades from the profile on top of base stats
    m_levelPlayer->applyBonusHealth   (profile.upgrades.bonusMaxHp);
    m_levelPlayer->applyBonusAttack   (profile.upgrades.bonusAttack);
    m_levelPlayer->applyBonusSpPerAtk (profile.upgrades.bonusSpPerAtk);
    m_combat.setPlayer(m_levelPlayer);

    // Build scene and spawn everything
    buildScene();
    placePlayer();
    spawnEnemies();

    // Update HUD
    m_goldHud->setGold(profile.gold);
    m_goldHud->raise();
    updatePlayerHud();
    positionPlayerHud();

    if (m_pauseOverlay)
        m_pauseOverlay->setGeometry(rect());

    m_ticker.start();
    setFocus();

    if (m_audio) m_audio->playMusic("/music/battle.ogg");
}

void Level1Widget::deactivate()
{
    m_ticker.stop();
    m_heldKeys.clear();
    m_paused = false;
    m_pauseOverlay->hide();
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
// Keep this switch — defines pathColor and titleText used below
    QColor pathColor;
    QString titleText;

    switch (m_level.id) {
    case 1: pathColor = QColor("#4a3728"); titleText = "GOBLIN CAVE";       break;
    case 2: pathColor = QColor("#1a3d1a"); titleText = "HAUNTED FOREST";    break;
    case 3: pathColor = QColor("#1a2f4a"); titleText = "FROZEN PEAK";       break;
    case 4: pathColor = QColor("#4a1a00"); titleText = "VOLCANIC DUNGEON";  break;
    case 5: pathColor = QColor("#2e1a4a"); titleText = "THE FINAL ARENA";   break;
    default: pathColor = QColor("#2a2a4a"); titleText = "LEVEL " + QString::number(m_level.id); break;
    }

    // ── Background image ──────────────────────────────────────────────────────
    QString bgPath;
    switch (m_level.id) {
    case 1: bgPath = ":/backgrounds/cave_level.jpg";     break;
    case 2: bgPath = ":/backgrounds/forest_level.jpg";   break;
    case 3: bgPath = ":/backgrounds/mountain_level.jpg"; break;
    case 4: bgPath = ":/backgrounds/volcano_level.jpg";  break;
    default: bgPath = "";                                  break;
    }

    if (!bgPath.isEmpty()) {
        QPixmap bg = SpriteCache::instance().get(bgPath)
                         .scaled(WORLD_W, WORLD_H,
                                 Qt::IgnoreAspectRatio,
                                 Qt::SmoothTransformation);
        m_scene->setBackgroundBrush(QBrush(bg));
    } else {
        m_scene->setBackgroundBrush(QBrush(QColor("#0A0A0A")));
    }   
    // ── Level title banner ────────────────────────────────────────────────────
    auto* banner = m_scene->addText(
        m_level.name.isEmpty() ? titleText : m_level.name.toUpper(),
        QFont("Press Start 2P", 9));
    banner->setDefaultTextColor(QColor("#FFD700"));
    banner->setPos((WORLD_W - banner->boundingRect().width()) / 2.0, 10);
    banner->setZValue(10);

    // ── Controls hint ─────────────────────────────────────────────────────────
    auto* hint = m_scene->addText(
        "WASD / Arrows = move   J = attack   K = special   ESC = pause",
        QFont("Arial", 6));
    hint->setDefaultTextColor(QColor("#ffffff66"));
    hint->setPos(8, WORLD_H - 18);
    hint->setZValue(10);

    // ── Player sprite ─────────────────────────────────────────────────────────
    m_player = new DungeonPlayerSprite(m_sheet, m_attackSheet);
    m_player->setZValue(9);
    m_scene->addItem(m_player);
}

void Level1Widget::placePlayer()
{
    if (m_player)
        m_player->setPos(
            (WORLD_W - DungeonPlayerSprite::W) / 2.0,
            WORLD_H - 140.0);
}

void Level1Widget::loadPlayerSheet(CharacterType type)
{
    QString path;
    switch (type) {
    case CharacterType::Mage:   path = ":/sprites/mage_movement_8dir_6frames.png";    break;
    case CharacterType::Archer: path = ":/sprites/archer_movement_8dir_6frames.png";  break;
    default:                    path = ":/sprites/warrior_movement_8dir_6frames.png"; break;
    }
    m_sheet.pixmap = SpriteCache::instance().get(path);
}

void Level1Widget::loadAttackSheet(CharacterType type)
{
    QString path;
    switch (type) {
    case CharacterType::Mage:   path = ":/sprites/mage_attack_8dir_7frames.png";    break;
    case CharacterType::Archer: path = ":/sprites/archer_attack_8dir_7frames.png";  break;
    default:                    path = ":/sprites/warrior_attack_8dir_7frames.png"; break;
    }
    m_attackSheet.pixmap = SpriteCache::instance().get(path);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Enemy spawning
// ─────────────────────────────────────────────────────────────────────────────

void Level1Widget::spawnEnemies()
{
    // ── Regular enemies ───────────────────────────────────────────────────────
    const QList<QPointF> spawnPoints = {
        { WORLD_W * 0.40, 220 },
        { WORLD_W * 0.55, 300 },
        { WORLD_W * 0.45, 380 }
    };

    QPixmap movSheet = SpriteCache::instance().get(ENEMY_MOVEMENT_SHEET);
    QPixmap atkSheet = SpriteCache::instance().get(ENEMY_ATTACK_SHEET);

    for (int i = 0; i < ENEMY_COUNT && i < spawnPoints.size(); ++i) {
        auto* sprite = new EnemySprite(CharacterType::Warrior, "Goblin");
        sprite->setMovementSheet(movSheet);
        sprite->setAttackSheet(atkSheet);
        sprite->setPos(spawnPoints[i]);
        sprite->setZValue(8);
        m_scene->addItem(sprite);

        auto* logic = new Enemy(CharacterType::Warrior, "Goblin",
                                ENEMY_HP, ENEMY_DAMAGE);
        m_enemies.append(sprite);
        m_enemyLogic.insert(sprite, logic);
        m_combat.registerEnemy(logic);
    }

    // ── Boss — positioned at the top of the path, hidden behind enemies ───────
    m_bossLogic = new Enemy(CharacterType::Warrior, m_level.bossName,
                            ENEMY_HP * BOSS_HP_MULTIPLIER, BOSS_DAMAGE);

    m_bossSprite = new EnemySprite(CharacterType::Warrior, m_level.bossName);
    m_bossSprite->setMovementSheet(movSheet);
    m_bossSprite->setAttackSheet(atkSheet);

    // Scale up visually to make the boss look imposing
    m_bossSprite->setScale(BOSS_SCALE);
    m_bossSprite->setPos(
        WORLD_W * 0.5 - (EnemySprite::W * BOSS_SCALE) / 2.0,
        80.0);
    m_bossSprite->setZValue(8);
    m_scene->addItem(m_bossSprite);
    // Boss is NOT registered with m_combat yet — only when m_bossActive becomes true
}

void Level1Widget::clearEnemies()
{
    for (EnemySprite* sprite : m_enemies) {
        m_scene->removeItem(sprite);
        Enemy* logic = m_enemyLogic.value(sprite, nullptr);
        m_combat.unregisterEnemy(logic);
        delete sprite;
        delete logic;
    }
    m_enemies.clear();
    m_enemyLogic.clear();
}

void Level1Widget::spawnExitPortal()
{
    const qreal pw = 80, ph = 56;
    const qreal px = (WORLD_W - pw) / 2.0;
    const qreal py = 30;

    // Stone border
    m_scene->addRect(px - 8, py - 6, pw + 16, ph + 10,
                     Qt::NoPen, QBrush(QColor("#1a3a1a")))->setZValue(5);

    // Portal zone (visible green rect)
    m_scene->addRect(px, py, pw, ph,
                     Qt::NoPen, QBrush(QColor("#00e676")))->setZValue(6);

    // Label
    auto* lbl = m_scene->addText("↑ EXIT", QFont("Arial", 8, QFont::Bold));
    lbl->setDefaultTextColor(QColor("#a5d6a7"));
    lbl->setPos(px + 10, py + ph + 4);
    lbl->setZValue(7);

    m_exitArea = QRectF(px, py, pw, ph);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Game loop
// ─────────────────────────────────────────────────────────────────────────────

void Level1Widget::onTick()
{
    if (m_paused || !m_player) return;

    // Update combat cooldowns (16ms per tick = 0.016s)
    m_combat.update(0.016f);

    movePlayer();
    patrolEnemies();
    checkCollisions();
    checkAttackCollisions();
    fitView();
    updatePlayerHud();
    positionPlayerHud();

    // Check if player is dead
    Character* playerChar = m_combat.player();
    if (playerChar && !playerChar->isAlive()) {
        deactivate();
        emit backToMenu();
    }
}

void Level1Widget::movePlayer()
{
    if (!m_player)
        return;

    QPointF velocity = m_movement.velocityFromKeys(
        m_heldKeys,
        DungeonPlayerSprite::W > 0 ? 3.0 : 3.0,
        GameplayMovementManager::ControlScheme::WasdAndArrows
    );
QRectF corridor(
        48,
        80.0,
        WORLD_W - 96,
        WORLD_H - 120.0
    );
    QPointF next = m_movement.resolveMovement(
        m_player->pos(),
        velocity,
        DungeonPlayerSprite::W,
        DungeonPlayerSprite::H,
        corridor
    );

    m_player->setPos(next);

    bool moving = velocity.x() != 0.0 || velocity.y() != 0.0;
    Direction dir = m_movement.directionFromVelocity(velocity, m_facing);
    m_player->updateAnimation(moving, dir);

    if (moving)
        m_facing = dir;
}

void Level1Widget::patrolEnemies()
{
    if (!m_player)
        return;

    QRectF worldBounds(0, 0, WORLD_W, WORLD_H);
    QRectF playerBounds = m_player->sceneBoundingRect();

    m_enemyMovement.updateEnemyMovement(m_enemies, playerBounds, worldBounds);

    if (m_bossActive && m_bossSprite && m_bossLogic && m_bossLogic->isAlive()) {
        QList<EnemySprite*> bossList;
        bossList.append(m_bossSprite);
        m_enemyMovement.updateEnemyMovement(bossList, playerBounds, worldBounds);
    }
}

void Level1Widget::checkCollisions()
{
    if (!m_player) return;

    // ── Exit portal ───────────────────────────────────────────────────────────
    if (m_exitArea.isValid() &&
        m_player->sceneBoundingRect().intersects(m_exitArea)) {
        deactivate();
        emit exitedLevel();
        return;
    }

    // Player hurt box — slightly smaller than sprite
    QRectF playerBox = m_player->sceneBoundingRect().adjusted(28, 28, -28, -12);

    // ── Boss contact ──────────────────────────────────────────────────────────
    if (m_bossActive && m_bossSprite && m_bossLogic && m_bossLogic->isAlive()) {
        QRectF bossBox = m_bossSprite->sceneBoundingRect().adjusted(14, 14, -14, -8);

        if (playerBox.intersects(bossBox) && m_combat.canEnemyAttack(m_bossLogic)) {
            m_combat.damagePlayer(m_bossLogic->damage());
            m_combat.startEnemyAttackCooldown(m_bossLogic);
            m_bossSprite->startAttackAnimation(m_bossSprite->facingDirection());
            if (m_audio) m_audio->playSfx("/sfx/hit.wav");
        }
        return;   // while boss is active, regular enemies don't attack
    }

    // ── Regular enemy contact ─────────────────────────────────────────────────
    for (EnemySprite* sprite : m_enemies) {
        if (!sprite) continue;
        Enemy* logic = m_enemyLogic.value(sprite, nullptr);
        if (!logic || !logic->isAlive()) continue;

        QRectF enemyBox = sprite->hitBox().adjusted(14, 14, -14, -8);

        if (playerBox.intersects(enemyBox) && m_combat.canEnemyAttack(logic)) {
            m_combat.damagePlayer(m_combat.enemyAttackDamage(logic));
            m_combat.startEnemyAttackCooldown(logic);
            sprite->startAttackAnimation(sprite->facingDirection());
            if (m_audio) m_audio->playSfx("/sfx/hit.wav");
        }
    }
}

void Level1Widget::checkAttackCollisions()
{
    const QList<ActiveAttack*>& attacks = m_combat.activeAttacks();

    for (ActiveAttack* attack : attacks) {
        if (!attack || attack->expired) continue;

        // ── Hit boss ──────────────────────────────────────────────────────────
        if (m_bossActive && m_bossSprite && m_bossLogic && m_bossLogic->isAlive()) {
            if (attack->bounds.intersects(m_bossSprite->sceneBoundingRect())
                && !attack->enemiesHit.contains(m_bossLogic)) {

                attack->enemiesHit.insert(m_bossLogic);
                m_bossLogic->takeDamage(attack->damage);
                if (m_audio) m_audio->playSfx("/sfx/hit.wav");

                if (!attack->piercing) m_combat.expireAttack(attack);

                if (!m_bossLogic->isAlive()) {
                    // Boss defeated — remove sprite, spawn exit portal
                    m_scene->removeItem(m_bossSprite);
                    delete m_bossSprite;
                    m_bossSprite = nullptr;
                    m_combat.unregisterEnemy(m_bossLogic);
                    if (m_audio) m_audio->playSfx("/sfx/faint.wav");
                    spawnExitPortal();
                }
            }
            continue;   // while boss alive, don't also hit regular enemies
        }

        // ── Hit regular enemies ───────────────────────────────────────────────
        for (EnemySprite* sprite : m_enemies) {
            if (!sprite) continue;
            Enemy* logic = m_enemyLogic.value(sprite, nullptr);
            if (!logic || !logic->isAlive()) continue;
            if (!attack->bounds.intersects(sprite->hitBox())) continue;
            if (attack->enemiesHit.contains(logic)) continue;

            attack->enemiesHit.insert(logic);
            m_combat.damageEnemy(logic, attack->damage);
            if (m_audio) m_audio->playSfx("/sfx/hit.wav");
            if (!attack->piercing) { m_combat.expireAttack(attack); break; }
        }
    }

    // ── Remove dead regular enemies ───────────────────────────────────────────
    for (int i = m_enemies.size() - 1; i >= 0; --i) {
        EnemySprite* sprite = m_enemies[i];
        if (!sprite) continue;
        Enemy* logic = m_enemyLogic.value(sprite, nullptr);
        if (!logic || !logic->isAlive()) {
            m_combat.addSpecialFromKill();
            if (m_audio) m_audio->playSfx("/sfx/faint.wav");
            m_combat.unregisterEnemy(logic);
            m_enemyLogic.remove(sprite);
            m_enemies.removeAt(i);
            m_scene->removeItem(sprite);
            delete sprite;
            delete logic;
        }
    }

    // ── All regular enemies dead → activate boss ──────────────────────────────
    if (m_enemies.isEmpty() && !m_bossActive && m_bossSprite) {
        m_bossActive = true;
        m_combat.registerEnemy(m_bossLogic);
        if (m_audio) m_audio->playSfx("/sfx/special.wav");

        // Pause the game and fire the boss dialog signal
        m_ticker.stop();
        emit bossTriggered(m_level);
    }
}

void Level1Widget::handleClassAttack()
{
    if (!m_player) return;
    QRectF playerBounds = m_player->sceneBoundingRect();
    ActiveAttack* attack = m_combat.createPlayerAttack(playerBounds, m_facing);
    if (attack) {
        m_player->startAttackAnimation(m_facing);
        if (m_audio) m_audio->playSfx("/sfx/special.wav");
    }
}

void Level1Widget::handleSpecialAbility()
{
    if (!m_player) return;
    QRectF playerBounds = m_player->sceneBoundingRect();
    const bool wasBefore = m_combat.warriorSpecialActive();
    QList<ActiveAttack*> attacks = m_combat.createPlayerSpecial(playerBounds, m_facing);
    const bool changedNow = wasBefore != m_combat.warriorSpecialActive();
    if (!attacks.isEmpty() || changedNow) {
        m_player->startAttackAnimation(m_facing);
        if (m_audio) m_audio->playSfx("/sfx/special.wav");
    }
    updatePlayerHud();
}

// ─────────────────────────────────────────────────────────────────────────────
//  HUD
// ─────────────────────────────────────────────────────────────────────────────

void Level1Widget::buildPlayerHud()
{
    if (m_playerHud) return;

    m_playerHud = new QWidget(this);
    m_playerHud->setFixedSize(250, 78);
    m_playerHud->setAttribute(Qt::WA_StyledBackground, true);
    m_playerHud->setStyleSheet(
        "QWidget { background-color: rgba(13,13,26,190); border: 3px solid #F0E8D0; }"
        "QLabel  { background: transparent; border: none; color: #F0E8D0; font-size: 8px; }");

    auto* root = new QVBoxLayout(m_playerHud);
    root->setContentsMargins(10, 8, 10, 8);
    root->setSpacing(6);

    auto* hpRow = new QHBoxLayout;
    m_healthLabel = new QLabel("HP", m_playerHud);
    m_healthLabel->setFixedWidth(60);
    m_healthBar = new HealthBarWidget(m_playerHud);
    m_healthBar->setFixedSize(160, 18);
    hpRow->addWidget(m_healthLabel);
    hpRow->addWidget(m_healthBar);

    auto* spRow = new QHBoxLayout;
    m_specialLabel = new QLabel("SPECIAL", m_playerHud);
    m_specialLabel->setFixedWidth(60);
    m_specialBar = new HealthBarWidget(m_playerHud);
    m_specialBar->setFixedSize(160, 18);
    m_specialBar->setFixedBarColor(QColor("#4A90D9"));
    spRow->addWidget(m_specialLabel);
    spRow->addWidget(m_specialBar);

    root->addLayout(hpRow);
    root->addLayout(spRow);
}

void Level1Widget::updatePlayerHud()
{
    if (!m_playerHud) return;
    Character* p = m_combat.player();
    if (!p) return;

    int currentHp = static_cast<int>(p->getHealthPercent() * p->getMaxHealth());
    m_healthLabel->setText(QString("HP %1/%2").arg(currentHp).arg(p->getMaxHealth()));

    m_healthBar->animateTo(p->getHealthPercent());

    float spPct = p->getSpPercent();
    m_specialLabel->setText(
        QString("SP %1%").arg(static_cast<int>(spPct * 100)));
    m_specialBar->animateTo(spPct);
}

void Level1Widget::positionPlayerHud()
{
    if (!m_playerHud) return;
    const int margin = 10;
    m_playerHud->move(margin, height() - m_playerHud->height() - margin);
    m_playerHud->raise();
}

// ─────────────────────────────────────────────────────────────────────────────
//  View
// ─────────────────────────────────────────────────────────────────────────────

void Level1Widget::fitView()
{
    if (!m_view || !m_player) return;
    m_view->resetTransform();
    m_view->scale(2.0, 2.0);
    m_view->centerOn(m_player);
}

void Level1Widget::togglePause()
{
    m_paused = !m_paused;
    if (!m_pauseOverlay) return;
    m_pauseOverlay->setGeometry(rect());
    if (m_paused) { m_pauseOverlay->show(); m_pauseOverlay->raise(); }
    else          { m_pauseOverlay->hide(); setFocus(); }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Input
// ─────────────────────────────────────────────────────────────────────────────

void Level1Widget::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape && !e->isAutoRepeat()) {
        togglePause();
        return;
    }

    if (m_paused) return;

    // Attack keys — same as dungeon (J = attack, K = special)
    // No autorepeat so holding J doesn't spam attacks
    if (e->key() == Qt::Key_J && !e->isAutoRepeat()) {
        handleClassAttack();
        return;
    }
    if (e->key() == Qt::Key_K && !e->isAutoRepeat()) {
        handleSpecialAbility();
        return;
    }

    // Movement keys — allow autorepeat so held keys keep firing
    if (!e->isAutoRepeat())
        m_heldKeys.insert(e->key());

    QWidget::keyPressEvent(e);
}

void Level1Widget::keyReleaseEvent(QKeyEvent* e)
{
    if (!e->isAutoRepeat())
        m_heldKeys.remove(e->key());
}

void Level1Widget::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    if (m_goldHud)
        m_goldHud->move(width() - m_goldHud->width() - 8, 8);
    if (m_pauseOverlay)
        m_pauseOverlay->setGeometry(rect());
    fitView();
}

void Level1Widget::resumeAfterDialog()
{
    m_ticker.start();
    setFocus();
}

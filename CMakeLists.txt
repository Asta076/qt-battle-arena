cmake_minimum_required(VERSION 3.19)
project(CS2-Battle-Arena LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia)
qt_standard_project_setup()

qt_add_executable(CS2-Battle-Arena
    WIN32 MACOSX_BUNDLE

    # ── Entry point ────────────────────────────
    src/main.cpp

    # ── Core character logic ───────────────────
    include/character.h          src/character.cpp
    include/warrior.h            src/warrior.cpp
    include/mage.h               src/mage.cpp
    include/archer.h             src/archer.cpp
    include/playerprofile.h      src/playerprofile.cpp

    # ── New combat system ──────────────────────
    include/enemy.h              src/enemy.cpp
    include/projectile.h         src/projectile.cpp
    include/worldcombatmanager.h src/worldcombatmanager.cpp

    # ── Game engine / helpers ──────────────────
    include/gameengine.h         src/gameengine.cpp
    include/spritecache.h        src/spritecache.cpp
    include/playercontroller.h   src/playercontroller.cpp
    include/audiomanager.h       src/audiomanager.cpp

    # ── Main window shell ──────────────────────
    include/mainwindow.h         src/mainwindow.cpp
    src/mainwindow.ui

    # ── Screens ────────────────────────────────
    include/startscreenwidget.h          src/startscreenwidget.cpp
    include/characterselectiondialog.h   src/characterselectiondialog.cpp
    src/characterselectiondialog.ui
    include/saveslotwidget.h             src/saveslotwidget.cpp
    include/pvpcharacterselectwidget.h   src/pvpcharacterselectwidget.cpp
    include/pvpcharacterselectwidget.h   src/pvpcharacterselectwidget.cpp

    # ── Level system ───────────────────────────────────────────────────────────
        include/leveldef.h
        include/levelmanager.h               src/levelmanager.cpp
        include/levelselectwidget.h          src/levelselectwidget.cpp
        include/storyslidedialog.h           src/storyslidedialog.cpp
        include/bossdialogwidget.h           src/bossdialogwidget.cpp
        include/level1widget.h               src/level1widget.cpp

    # ── Shared widgets ─────────────────────────
    include/goldhudwidget.h        src/goldhudwidget.cpp
    include/healthbarwidget.h      src/healthbarwidget.cpp
    include/pauseoverlaywidget.h   src/pauseoverlaywidget.cpp

    # ── World / PvP widgets ────────────────────
    include/overworldwidget.h      src/overworldwidget.cpp
    include/dungeonwidget.h        src/dungeonwidget.cpp
    include/housewidget.h          src/housewidget.cpp
    include/shopwidget.h           src/shopwidget.cpp
    include/pvparenawidget.h       src/pvparenawidget.cpp
    src/shopwidget.ui

)

target_include_directories(CS2-Battle-Arena PRIVATE include)

qt_add_resources(CS2-Battle-Arena "assets"
    PREFIX "/"
    BASE resources
    FILES
        # Fonts / style
        resources/fonts/PressStart2P-Regular.ttf
        resources/styles/game.qss

        # Character front/back sprites
        resources/sprites/warrior_front.png
        resources/sprites/warrior_back.png
        resources/sprites/mage_front.png
        resources/sprites/mage_back.png
        resources/sprites/archer_front.png
        resources/sprites/archer_back.png

        # Movement sprite sheets
        resources/sprites/archer_movement_8dir_6frames.png
        resources/sprites/mage_movement_8dir_6frames.png
        resources/sprites/warrior_movement_8dir_6frames.png

        # attack sprites
        resources/sprites/warrior_attack_8dir_7frames.png
        resources/sprites/mage_attack_8dir_7frames.png
        resources/sprites/archer_attack_8dir_7frames.png

        # Old/fallback player sheet
        resources/sprites/player.png

        # Combat projectiles
        resources/sprites/arrow.png
        resources/sprites/fireball.png

        # World sprites
        resources/sprites/grass.png
        resources/sprites/dirt1.png
        resources/sprites/dirt2.png
        resources/sprites/house.png
        resources/sprites/tree_round.png
        resources/sprites/tree_pine.png
        resources/sprites/dirt_wall.png
        resources/sprites/shop.png
        resources/sprites/dungeon_entrance.png

        # Backgrounds
        resources/backgrounds/battle_bg.png
        resources/backgrounds/pvp_arena.png
        resources/backgrounds/dungeon_bg.png

        # Music
        resources/music/battle.ogg
        resources/music/menu.ogg
        resources/music/overworld.ogg

        # SFX
        resources/sfx/hit.wav
        resources/sfx/faint.wav
        resources/sfx/special.wav

        #enemies 
        resources/sprites/goblin_movement_8dir_6frames.png
        resources/sprites/goblin_attack_8dir_7frames.png

        resources/sprites/goblin_brute_movement_8dir_6frames.png
        resources/sprites/goblin_brute_attack_8dir_7frames.png

        resources/sprites/skeleton_guard_movement_8dir_6frames.png
        resources/sprites/skeleton_guard_attack_8dir_7frames.png
)

target_link_libraries(CS2-Battle-Arena
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Multimedia
)

include(GNUInstallDirs)

install(TARGETS CS2-Battle-Arena
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

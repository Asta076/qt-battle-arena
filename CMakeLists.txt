cmake_minimum_required(VERSION 3.19)

project(CS2-Battle-Arena LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia)

qt_standard_project_setup()

qt_add_executable(CS2-Battle-Arena
        WIN32 MACOSX_BUNDLE

        # ── Entry point ─────────────────────────────────────────────
        src/main.cpp

        # ── Core game data / models ─────────────────────────────────
        include/character.h
        src/character.cpp

        include/warrior.h
        src/warrior.cpp

        include/mage.h
        src/mage.cpp

        include/archer.h
        src/archer.cpp

        include/enemy.h
        src/enemy.cpp

        include/projectile.h
        src/projectile.cpp

        include/arrow.h
        src/arrow.cpp

        include/fireball.h
        src/fireball.cpp

        include/playerprofile.h
        src/playerprofile.cpp

        include/gameengine.h
        src/gameengine.cpp

        include/direction.h

        # ── Shared systems / managers ───────────────────────────────
        include/worldcombatmanager.h
        src/worldcombatmanager.cpp

        include/gamecombatmanager.h
        src/gamecombatmanager.cpp

        include/gameplaymovementmanager.h
        src/gameplaymovementmanager.cpp

        include/overworldlogicmanager.h
        src/overworldlogicmanager.cpp

        include/dungeonrunmanager.h
        src/dungeonrunmanager.cpp

        include/pvplogicmanager.h
        src/pvplogicmanager.cpp

        include/spritecache.h
        src/spritecache.cpp

        include/audiomanager.h
        src/audiomanager.cpp

        # ── Main window / navigation ────────────────────────────────
        include/mainwindow.h
        src/mainwindow.cpp
        src/mainwindow.ui

        # ── Start / save / character select ─────────────────────────
        include/startscreenwidget.h
        src/startscreenwidget.cpp

        include/saveslotwidget.h
        src/saveslotwidget.cpp

        include/characterselectiondialog.h
        src/characterselectiondialog.cpp
        src/characterselectiondialog.ui

        # ── HUD / overlays ──────────────────────────────────────────
        include/goldhudwidget.h
        src/goldhudwidget.cpp

        include/healthbarwidget.h
        src/healthbarwidget.cpp

        include/pauseoverlaywidget.h
        src/pauseoverlaywidget.cpp

        include/gameoverwidget.h
        src/gameoverwidget.cpp

        # ── Overworld / dungeon / house / shop ──────────────────────
        include/overworldwidget.h
        src/overworldwidget.cpp

        include/dungeonwidget.h
        src/dungeonwidget.cpp

        include/housewidget.h
        src/housewidget.cpp

        include/shopwidget.h
        src/shopwidget.cpp
        src/shopwidget.ui

        # ── Level system ────────────────────────────────────────────
        include/leveldef.h

        include/levelmanager.h
        src/levelmanager.cpp

        include/levelselectwidget.h
        src/levelselectwidget.cpp

        include/level1widget.h
        src/level1widget.cpp

        include/storyslidedialog.h
        src/storyslidedialog.cpp

        include/bossdialogwidget.h
        src/bossdialogwidget.cpp

        # ── PvP system ──────────────────────────────────────────────
        include/pvpcharacterselectwidget.h
        src/pvpcharacterselectwidget.cpp

        include/pvparenawidget.h
        src/pvparenawidget.cpp
)

target_include_directories(CS2-Battle-Arena
        PRIVATE
        include
)

qt_add_resources(CS2-Battle-Arena "assets"
        PREFIX "/"
        BASE resources
        FILES
        # ── Fonts / styles ──────────────────────────────────────
        resources/fonts/PressStart2P-Regular.ttf
        resources/styles/game.qss

        # ── Music ───────────────────────────────────────────────
        resources/music/battle.ogg
        resources/music/menu.ogg
        resources/music/overworld.ogg

        # ── SFX ─────────────────────────────────────────────────
        resources/sfx/hit.wav
        resources/sfx/faint.wav
        resources/sfx/special.wav

        # ── Backgrounds ─────────────────────────────────────────
        resources/backgrounds/battle_bg.png
        resources/backgrounds/dungeon_bg.png
        resources/backgrounds/pvp_arena.png
        resources/backgrounds/cave_level.jpg
        resources/backgrounds/forest_level.jpg
        resources/backgrounds/mountain_level.jpg
        resources/backgrounds/volcano_level.jpg
        # ── Basic sprites ───────────────────────────────────────
        resources/sprites/player.png

        resources/sprites/warrior_front.png
        resources/sprites/warrior_back.png
        resources/sprites/mage_front.png
        resources/sprites/mage_back.png
        resources/sprites/archer_front.png
        resources/sprites/archer_back.png

        resources/sprites/grass.png
        resources/sprites/dirt1.png
        resources/sprites/dirt2.png
        resources/sprites/dirt_wall.png

        resources/sprites/house.png
        resources/sprites/shop.png
        resources/sprites/level_selector.png
        resources/sprites/tree_round.png
        resources/sprites/tree_pine.png
        resources/sprites/dungeon_entrance.png

        # ── Projectiles ─────────────────────────────────────────
        resources/sprites/arrow.png
        resources/sprites/fireball.png

        # ── Player movement sheets ──────────────────────────────
        resources/sprites/warrior_movement_8dir_6frames.png
        resources/sprites/mage_movement_8dir_6frames.png
        resources/sprites/archer_movement_8dir_6frames.png

        # ── Player attack sheets ────────────────────────────────
        resources/sprites/warrior_attack_8dir_7frames.png
        resources/sprites/mage_attack_8dir_7frames.png
        resources/sprites/archer_attack_8dir_7frames.png

        # ── Enemy movement sheets ───────────────────────────────
        resources/sprites/skeleton_guard_movement_8dir_6frames.png
        resources/sprites/goblin_brute_movement_8dir_6frames.png
        resources/sprites/goblin_movement_8dir_6frames.png

        # ── Enemy attack sheets ─────────────────────────────────
        resources/sprites/skeleton_guard_attack_8dir_7frames.png
        resources/sprites/goblin_brute_attack_8dir_7frames.png
        resources/sprites/goblin_attack_8dir_7frames.png
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

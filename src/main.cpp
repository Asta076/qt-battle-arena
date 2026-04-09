#include <QApplication>
#include <QFontDatabase>
#include <QFile>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Load the pixel font from resources
    QFontDatabase::addApplicationFont(":/fonts/PressStart2P.ttf");

    // Load and apply the global stylesheet
    QFile styleFile(":/styles/game.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        app.setStyleSheet(styleFile.readAll());
        styleFile.close();
    }

    MainWindow window;
    window.setWindowTitle("CSCE 1101 Battle Arena");
    window.resize(960, 640);
    window.show();

    return app.exec();
}

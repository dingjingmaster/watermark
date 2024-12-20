//
// Created by dingjing on 12/20/24.
//

#include "water-mark.h"

#include <QApplication>
#include <QGuiApplication>

void set_watermark(const QString& str, bool othersInfo)
{
    static int argc = 1;
    static char* argv[] = { "water-marker" };
    QApplication app(argc, argv);

    DesktopManager::getInstance()->setString (str, othersInfo);
    DesktopManager::getInstance()->run();

    app.exec();
}

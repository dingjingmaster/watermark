//
// Created by dingjing on 23-6-12.
//
#include "singleton-app-gui.h"

#include "desktop-manager.h"

int main (int argc, char* argv[])
{
    SingletonApp app(argc, argv, "watermark-desktop");

    if (app.getString().isEmpty()) {
        app.showHelp();
        ::exit (1);
    }

    DesktopManager::getInstance()->setString (app.getString());
    DesktopManager::getInstance()->run();

    return SingletonApp::exec();
}
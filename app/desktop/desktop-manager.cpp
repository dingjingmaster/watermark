//
// Created by dingjing on 23-6-12.
//

#include "desktop-manager.h"

#include <glib.h>

#include <QGuiApplication>

#include "screen.h"


DesktopManager* DesktopManager::gInstance = nullptr;

DesktopManager *DesktopManager::getInstance()
{
    static gint64 lc = 0;

    if (g_once_init_enter(&lc)) {
        if (!gInstance) {
            gInstance = new DesktopManager;
            g_once_init_leave(&lc, 1);
        }
    }

    return gInstance;
}

DesktopManager::DesktopManager(QObject *parent)
    : QObject (parent)
{
    auto screens = qApp->screens();

    auto font = qApp->font();
    font.setBold (true);
    font.setPointSize (18);
    font.setStyleStrategy (QFont::ForceOutline);
    qApp->setFont (font);

    for (auto s : screens) {
        mScreens.append (new Screen(mString, s));
    }
}

void DesktopManager::setString(const QString &str)
{
    mString = str;

    for (auto s : mScreens) {
        s->setString (mString);
    }
}

void DesktopManager::run()
{
    for (auto s : mScreens) {
        s->showWindow();
    }
}
